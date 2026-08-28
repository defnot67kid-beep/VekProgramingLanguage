#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

bool VekValue::IsNil() const { return std::holds_alternative<std::monostate>(value); }
bool VekValue::IsNumber() const { return std::holds_alternative<double>(value); }
bool VekValue::IsBool() const { return std::holds_alternative<bool>(value); }
bool VekValue::IsString() const { return std::holds_alternative<std::string>(value); }

double VekValue::AsNumber(double fallback) const {
    if (auto p = std::get_if<double>(&value)) return *p;
    if (auto p = std::get_if<bool>(&value)) return *p ? 1.0 : 0.0;
    if (auto p = std::get_if<std::string>(&value)) {
        try { std::size_t n = 0; double x = std::stod(*p, &n); return n == p->size() ? x : fallback; }
        catch (...) { return fallback; }
    }
    return fallback;
}

bool VekValue::AsBool(bool fallback) const {
    if (auto p = std::get_if<bool>(&value)) return *p;
    if (auto p = std::get_if<double>(&value)) return std::fabs(*p) > 1e-12;
    if (auto p = std::get_if<std::string>(&value)) return !p->empty();
    if (IsNil()) return false;
    return fallback;
}

std::string VekValue::AsString() const {
    if (auto p = std::get_if<std::string>(&value)) return *p;
    if (auto p = std::get_if<bool>(&value)) return *p ? "true" : "false";
    if (auto p = std::get_if<double>(&value)) {
        std::ostringstream out;
        out.precision(15);
        out << *p;
        return out.str();
    }
    return "nil";
}

bool VekValue::Truthy() const { return AsBool(false); }

namespace {

enum class TokenType {
    End, Identifier, Number, String,
    Fn, If, Else, Let, While, Break, Continue, Return, True, False, Nil,
    LeftParen, RightParen, LeftBrace, RightBrace,
    Comma, Semicolon,
    Plus, Minus, Star, Slash, Percent,
    Bang, BangEqual, Equal, EqualEqual,
    Greater, GreaterEqual, Less, LessEqual,
    AndAnd, OrOr
};

struct Token {
    TokenType type = TokenType::End;
    std::string lexeme;
    double number = 0.0;
    int line = 1;
};

class Lexer {
public:
    Lexer(const std::string& src, const VekSecurityPolicy& limits) : source(src), policy(limits) {}

    std::vector<Token> Scan() {
        std::vector<Token> out;
        while (!AtEnd()) {
            start = current;
            ScanToken(out);
            if (out.size() > policy.maxTokens) throw std::runtime_error("VEK security: token count exceeds limit");
        }
        out.push_back({TokenType::End, "", 0.0, line});
        return out;
    }

private:
    const std::string& source;
    const VekSecurityPolicy& policy;
    std::size_t start = 0, current = 0;
    int line = 1;

    bool AtEnd() const { return current >= source.size(); }
    char Advance() { return source[current++]; }
    char Peek() const { return AtEnd() ? '\0' : source[current]; }
    char PeekNext() const { return current + 1 >= source.size() ? '\0' : source[current + 1]; }
    bool Match(char expected) { if (AtEnd() || source[current] != expected) return false; ++current; return true; }

    void Add(std::vector<Token>& out, TokenType type) {
        out.push_back({type, source.substr(start, current - start), 0.0, line});
    }

    void ScanToken(std::vector<Token>& out) {
        char c = Advance();
        switch (c) {
            case '(': Add(out, TokenType::LeftParen); break;
            case ')': Add(out, TokenType::RightParen); break;
            case '{': Add(out, TokenType::LeftBrace); break;
            case '}': Add(out, TokenType::RightBrace); break;
            case ',': Add(out, TokenType::Comma); break;
            case ';': Add(out, TokenType::Semicolon); break;
            case '+': Add(out, TokenType::Plus); break;
            case '-': Add(out, TokenType::Minus); break;
            case '*': Add(out, TokenType::Star); break;
            case '%': Add(out, TokenType::Percent); break;
            case '!': Add(out, Match('=') ? TokenType::BangEqual : TokenType::Bang); break;
            case '=': Add(out, Match('=') ? TokenType::EqualEqual : TokenType::Equal); break;
            case '>': Add(out, Match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
            case '<': Add(out, Match('=') ? TokenType::LessEqual : TokenType::Less); break;
            case '&': if (Match('&')) Add(out, TokenType::AndAnd); else Error("expected '&' after '&'"); break;
            case '|': if (Match('|')) Add(out, TokenType::OrOr); else Error("expected '|' after '|'"); break;
            case '/':
                if (Match('/')) { while (Peek() != '\n' && !AtEnd()) Advance(); }
                else Add(out, TokenType::Slash);
                break;
            case '#': while (Peek() != '\n' && !AtEnd()) Advance(); break;
            case ' ': case '\r': case '\t': break;
            case '\n': ++line; break;
            case '"': String(out); break;
            default:
                if (std::isdigit(static_cast<unsigned char>(c))) Number(out);
                else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') Identifier(out);
                else Error("unexpected character");
        }
    }

    [[noreturn]] void Error(const std::string& what) const {
        throw std::runtime_error("VEK lexer line " + std::to_string(line) + ": " + what);
    }

    void String(std::vector<Token>& out) {
        std::string value;
        while (!AtEnd() && Peek() != '"') {
            char c = Advance();
            if (c == '\n') ++line;
            if (c == '\\' && !AtEnd()) {
                char e = Advance();
                switch (e) {
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    default: value.push_back(e); break;
                }
            } else value.push_back(c);
            if (value.size() > policy.maxStringBytes) Error("string literal exceeds configured limit");
        }
        if (AtEnd()) Error("unterminated string");
        Advance();
        out.push_back({TokenType::String, value, 0.0, line});
    }

    void Number(std::vector<Token>& out) {
        while (std::isdigit(static_cast<unsigned char>(Peek()))) Advance();
        if (Peek() == '.' && std::isdigit(static_cast<unsigned char>(PeekNext()))) {
            Advance();
            while (std::isdigit(static_cast<unsigned char>(Peek()))) Advance();
        }
        std::string text = source.substr(start, current - start);
        out.push_back({TokenType::Number, text, std::stod(text), line});
    }

    void Identifier(std::vector<Token>& out) {
        while (std::isalnum(static_cast<unsigned char>(Peek())) || Peek() == '_') Advance();
        std::string text = source.substr(start, current - start);
        static const std::unordered_map<std::string, TokenType> words = {
            {"fn",TokenType::Fn},{"if",TokenType::If},{"else",TokenType::Else},{"let",TokenType::Let},
            {"while",TokenType::While},{"break",TokenType::Break},{"continue",TokenType::Continue},
            {"return",TokenType::Return},{"true",TokenType::True},{"false",TokenType::False},{"nil",TokenType::Nil}
        };
        auto it = words.find(text);
        out.push_back({it == words.end() ? TokenType::Identifier : it->second, text, 0.0, line});
    }
};

struct Expr {
    enum class Kind { Literal, Variable, Unary, Binary, Call } kind = Kind::Literal;
    VekValue literal;
    std::string text;
    TokenType op = TokenType::End;
    std::unique_ptr<Expr> left, right;
    std::vector<std::unique_ptr<Expr>> args;
};

struct Stmt {
    enum class Kind { Expression, Let, Assign, If, While, Break, Continue, Return } kind = Kind::Expression;
    std::string name;
    std::unique_ptr<Expr> expression;
    std::vector<Stmt> thenBranch, elseBranch;
};

struct FunctionDef { std::vector<std::string> params; std::vector<Stmt> body; };

class Parser {
public:
    Parser(std::vector<Token> input, const VekSecurityPolicy& limits) : tokens(std::move(input)), policy(limits) {}

    std::unordered_map<std::string, FunctionDef> ParseProgram() {
        std::unordered_map<std::string, FunctionDef> functions;
        while (!Check(TokenType::End)) {
            if (functions.size() >= policy.maxFunctions) throw std::runtime_error("VEK security: function count exceeds limit");
            Consume(TokenType::Fn, "expected 'fn'");
            Token name = Consume(TokenType::Identifier, "expected function name");
            Consume(TokenType::LeftParen, "expected '('");
            FunctionDef fn;
            if (!Check(TokenType::RightParen)) {
                do {
                    if (fn.params.size() >= policy.maxParametersPerFunction) Error("too many function parameters");
                    fn.params.push_back(Consume(TokenType::Identifier, "expected parameter name").lexeme);
                } while (Match(TokenType::Comma));
            }
            Consume(TokenType::RightParen, "expected ')'");
            fn.body = ParseBlock();
            functions[name.lexeme] = std::move(fn);
        }
        return functions;
    }

private:
    std::vector<Token> tokens;
    const VekSecurityPolicy& policy;
    std::size_t current = 0;

    bool Check(TokenType t) const { return tokens[current].type == t; }
    bool CheckNext(TokenType t) const { return current + 1 < tokens.size() && tokens[current + 1].type == t; }
    bool AtEnd() const { return Check(TokenType::End); }
    Token Advance() { if (!AtEnd()) ++current; return tokens[current - 1]; }
    bool Match(TokenType t) { if (!Check(t)) return false; Advance(); return true; }
    [[noreturn]] void Error(const std::string& m) const { throw std::runtime_error("VEK parser line " + std::to_string(tokens[current].line) + ": " + m); }
    Token Consume(TokenType t, const std::string& m) { if (Check(t)) return Advance(); Error(m); }

    std::vector<Stmt> ParseBlock() {
        Consume(TokenType::LeftBrace, "expected '{'");
        std::vector<Stmt> body;
        while (!Check(TokenType::RightBrace) && !AtEnd()) body.push_back(ParseStatement());
        Consume(TokenType::RightBrace, "expected '}'");
        return body;
    }

    Stmt ParseStatement() {
        if (Match(TokenType::If)) return ParseIf();
        if (Match(TokenType::While)) return ParseWhile();
        if (Match(TokenType::Let)) return ParseLet();
        if (Match(TokenType::Break)) { Stmt s; s.kind=Stmt::Kind::Break; Consume(TokenType::Semicolon,"expected ';' after break"); return s; }
        if (Match(TokenType::Continue)) { Stmt s; s.kind=Stmt::Kind::Continue; Consume(TokenType::Semicolon,"expected ';' after continue"); return s; }
        if (Match(TokenType::Return)) return ParseReturn();
        if (Check(TokenType::Identifier) && CheckNext(TokenType::Equal)) return ParseAssign();
        Stmt s; s.kind=Stmt::Kind::Expression; s.expression=ParseExpression(); Consume(TokenType::Semicolon,"expected ';' after expression"); return s;
    }

    Stmt ParseIf() {
        Stmt s; s.kind=Stmt::Kind::If; s.expression=ParseExpression(); s.thenBranch=ParseBlock();
        if (Match(TokenType::Else)) {
            if (Match(TokenType::If)) { Stmt nested=ParseIf(); s.elseBranch.push_back(std::move(nested)); }
            else s.elseBranch=ParseBlock();
        }
        return s;
    }
    Stmt ParseWhile() { Stmt s; s.kind=Stmt::Kind::While; s.expression=ParseExpression(); s.thenBranch=ParseBlock(); return s; }
    Stmt ParseLet() { Stmt s; s.kind=Stmt::Kind::Let; s.name=Consume(TokenType::Identifier,"expected variable name").lexeme; Consume(TokenType::Equal,"expected '=' after variable name"); s.expression=ParseExpression(); Consume(TokenType::Semicolon,"expected ';'"); return s; }
    Stmt ParseAssign() { Stmt s; s.kind=Stmt::Kind::Assign; s.name=Advance().lexeme; Consume(TokenType::Equal,"expected '='"); s.expression=ParseExpression(); Consume(TokenType::Semicolon,"expected ';'"); return s; }
    Stmt ParseReturn() { Stmt s; s.kind=Stmt::Kind::Return; if(!Check(TokenType::Semicolon)) s.expression=ParseExpression(); Consume(TokenType::Semicolon,"expected ';'"); return s; }

    std::unique_ptr<Expr> ParseExpression() { return ParseOr(); }
    std::unique_ptr<Expr> ParseOr() { auto e=ParseAnd(); while(Match(TokenType::OrOr)) e=Binary(std::move(e),TokenType::OrOr,ParseAnd()); return e; }
    std::unique_ptr<Expr> ParseAnd() { auto e=ParseEquality(); while(Match(TokenType::AndAnd)) e=Binary(std::move(e),TokenType::AndAnd,ParseEquality()); return e; }
    std::unique_ptr<Expr> ParseEquality() { auto e=ParseComparison(); while(Check(TokenType::EqualEqual)||Check(TokenType::BangEqual)){auto o=Advance().type;e=Binary(std::move(e),o,ParseComparison());} return e; }
    std::unique_ptr<Expr> ParseComparison() { auto e=ParseTerm(); while(Check(TokenType::Greater)||Check(TokenType::GreaterEqual)||Check(TokenType::Less)||Check(TokenType::LessEqual)){auto o=Advance().type;e=Binary(std::move(e),o,ParseTerm());} return e; }
    std::unique_ptr<Expr> ParseTerm() { auto e=ParseFactor(); while(Check(TokenType::Plus)||Check(TokenType::Minus)){auto o=Advance().type;e=Binary(std::move(e),o,ParseFactor());} return e; }
    std::unique_ptr<Expr> ParseFactor() { auto e=ParseUnary(); while(Check(TokenType::Star)||Check(TokenType::Slash)||Check(TokenType::Percent)){auto o=Advance().type;e=Binary(std::move(e),o,ParseUnary());} return e; }
    std::unique_ptr<Expr> ParseUnary() { if(Check(TokenType::Bang)||Check(TokenType::Minus)){auto o=Advance().type;auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Unary;e->op=o;e->right=ParseUnary();return e;} return ParsePrimary(); }

    std::unique_ptr<Expr> ParsePrimary() {
        if(Match(TokenType::True)) return Literal(VekValue(true));
        if(Match(TokenType::False)) return Literal(VekValue(false));
        if(Match(TokenType::Nil)) return Literal(VekValue());
        if(Check(TokenType::Number)) return Literal(VekValue(Advance().number));
        if(Check(TokenType::String)) return Literal(VekValue(Advance().lexeme));
        if(Check(TokenType::Identifier)) {
            Token name=Advance();
            if(Match(TokenType::LeftParen)) {
                auto call=std::make_unique<Expr>(); call->kind=Expr::Kind::Call; call->text=name.lexeme;
                if(!Check(TokenType::RightParen)) do { if(call->args.size()>=policy.maxArgumentsPerCall) Error("too many call arguments"); call->args.push_back(ParseExpression()); } while(Match(TokenType::Comma));
                Consume(TokenType::RightParen,"expected ')' after arguments"); return call;
            }
            auto v=std::make_unique<Expr>(); v->kind=Expr::Kind::Variable; v->text=name.lexeme; return v;
        }
        if(Match(TokenType::LeftParen)){auto e=ParseExpression();Consume(TokenType::RightParen,"expected ')'");return e;}
        Error("expected expression");
    }

    static std::unique_ptr<Expr> Literal(VekValue v){auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Literal;e->literal=std::move(v);return e;}
    static std::unique_ptr<Expr> Binary(std::unique_ptr<Expr> l,TokenType o,std::unique_ptr<Expr> r){auto e=std::make_unique<Expr>();e->kind=Expr::Kind::Binary;e->left=std::move(l);e->right=std::move(r);e->op=o;return e;}
};

bool ValuesEqual(const VekValue& a,const VekValue& b){
    if(a.IsNil()||b.IsNil()) return a.IsNil()&&b.IsNil();
    if(a.IsNumber()&&b.IsNumber()) return std::fabs(a.AsNumber()-b.AsNumber())<1e-9;
    if(a.IsBool()||b.IsBool()) return a.AsBool()==b.AsBool();
    return a.AsString()==b.AsString();
}

} // namespace

struct VekScriptEngine::Impl {
    std::unordered_map<std::string, FunctionDef> functions;
    std::unordered_map<std::string, NativeFunction> natives;
    std::string lastError, sourceName;
    bool loaded=false, nativesSealed=false;
    VekSecurityPolicy policy;
    std::size_t instructionsRemaining=0,nativeCallsRemaining=0,callDepth=0,loopIterationsRemaining=0;

    using Environment=std::unordered_map<std::string,VekValue>;
    enum class Flow { Normal, Return, Break, Continue };
    struct ExecResult { Flow flow=Flow::Normal; VekValue value; };

    void ConsumeInstruction(){if(instructionsRemaining==0)throw std::runtime_error("VEK security: instruction budget exceeded");--instructionsRemaining;}

    VekValue Eval(const Expr& e,Environment& env){
        ConsumeInstruction();
        switch(e.kind){
            case Expr::Kind::Literal:return e.literal;
            case Expr::Kind::Variable:{auto it=env.find(e.text);if(it==env.end())throw std::runtime_error("VEK runtime: unknown variable '"+e.text+"'");return it->second;}
            case Expr::Kind::Unary:{auto r=Eval(*e.right,env);if(e.op==TokenType::Bang)return VekValue(!r.Truthy());if(e.op==TokenType::Minus)return VekValue(-r.AsNumber());return {};}
            case Expr::Kind::Binary:{
                if(e.op==TokenType::AndAnd){auto l=Eval(*e.left,env);return l.Truthy()?VekValue(Eval(*e.right,env).Truthy()):VekValue(false);}
                if(e.op==TokenType::OrOr){auto l=Eval(*e.left,env);return l.Truthy()?VekValue(true):VekValue(Eval(*e.right,env).Truthy());}
                auto l=Eval(*e.left,env),r=Eval(*e.right,env);
                switch(e.op){
                    case TokenType::Plus: if(l.IsString()||r.IsString())return VekValue(l.AsString()+r.AsString()); return VekValue(l.AsNumber()+r.AsNumber());
                    case TokenType::Minus:return VekValue(l.AsNumber()-r.AsNumber());
                    case TokenType::Star:return VekValue(l.AsNumber()*r.AsNumber());
                    case TokenType::Slash:{double d=r.AsNumber();if(std::fabs(d)<1e-12)throw std::runtime_error("VEK runtime: division by zero");return VekValue(l.AsNumber()/d);}
                    case TokenType::Percent:{double d=r.AsNumber();if(std::fabs(d)<1e-12)throw std::runtime_error("VEK runtime: modulo by zero");return VekValue(std::fmod(l.AsNumber(),d));}
                    case TokenType::EqualEqual:return VekValue(ValuesEqual(l,r));
                    case TokenType::BangEqual:return VekValue(!ValuesEqual(l,r));
                    case TokenType::Greater:return VekValue(l.AsNumber()>r.AsNumber());
                    case TokenType::GreaterEqual:return VekValue(l.AsNumber()>=r.AsNumber());
                    case TokenType::Less:return VekValue(l.AsNumber()<r.AsNumber());
                    case TokenType::LessEqual:return VekValue(l.AsNumber()<=r.AsNumber());
                    default:return {};
                }
            }
            case Expr::Kind::Call:{
                std::vector<VekValue> args;args.reserve(e.args.size());for(auto& a:e.args)args.push_back(Eval(*a,env));
                auto n=natives.find(e.text);if(n!=natives.end()){if(nativeCallsRemaining==0)throw std::runtime_error("VEK security: native-call budget exceeded");--nativeCallsRemaining;return n->second(args);} return CallFunction(e.text,args);
            }
        }
        return {};
    }

    ExecResult ExecuteBlock(const std::vector<Stmt>& body,Environment& env,bool inLoop=false){
        for(const auto& s:body){
            ConsumeInstruction();
            switch(s.kind){
                case Stmt::Kind::Expression:Eval(*s.expression,env);break;
                case Stmt::Kind::Let:env[s.name]=Eval(*s.expression,env);break;
                case Stmt::Kind::Assign:{auto it=env.find(s.name);if(it==env.end())throw std::runtime_error("VEK runtime: assignment to unknown variable '"+s.name+"'");it->second=Eval(*s.expression,env);break;}
                case Stmt::Kind::If:{auto r=ExecuteBlock(Eval(*s.expression,env).Truthy()?s.thenBranch:s.elseBranch,env,inLoop);if(r.flow!=Flow::Normal)return r;break;}
                case Stmt::Kind::While:{
                    while(Eval(*s.expression,env).Truthy()){
                        if(loopIterationsRemaining==0)throw std::runtime_error("VEK security: loop iteration budget exceeded");--loopIterationsRemaining;
                        auto r=ExecuteBlock(s.thenBranch,env,true);
                        if(r.flow==Flow::Return)return r;
                        if(r.flow==Flow::Break)break;
                        if(r.flow==Flow::Continue)continue;
                    } break;
                }
                case Stmt::Kind::Break:if(!inLoop)throw std::runtime_error("VEK runtime: break used outside loop");return {Flow::Break,{}};
                case Stmt::Kind::Continue:if(!inLoop)throw std::runtime_error("VEK runtime: continue used outside loop");return {Flow::Continue,{}};
                case Stmt::Kind::Return:{ExecResult r;r.flow=Flow::Return;if(s.expression)r.value=Eval(*s.expression,env);return r;}
            }
        }
        return {};
    }

    VekValue CallFunction(const std::string& name,const std::vector<VekValue>& args){
        if(callDepth>=policy.maxCallDepth)throw std::runtime_error("VEK security: maximum call depth exceeded");
        auto it=functions.find(name);if(it==functions.end())throw std::runtime_error("VEK runtime: unknown function '"+name+"'");
        if(args.size()!=it->second.params.size())throw std::runtime_error("VEK runtime: function '"+name+"' expected "+std::to_string(it->second.params.size())+" args, received "+std::to_string(args.size()));
        ++callDepth;struct Guard{std::size_t& d;~Guard(){--d;}}g{callDepth};
        Environment env;for(std::size_t i=0;i<args.size();++i)env[it->second.params[i]]=args[i];
        return ExecuteBlock(it->second.body,env).value;
    }
};

VekScriptEngine::VekScriptEngine():impl(std::make_unique<Impl>()){}
VekScriptEngine::~VekScriptEngine()=default;
VekScriptEngine::VekScriptEngine(VekScriptEngine&&) noexcept=default;
VekScriptEngine& VekScriptEngine::operator=(VekScriptEngine&&) noexcept=default;

bool VekScriptEngine::LoadFile(const std::string& path){std::ifstream in(path,std::ios::binary);if(!in){impl->lastError="VEK: could not open script: "+path;impl->loaded=false;return false;}std::ostringstream s;s<<in.rdbuf();return LoadSource(s.str(),path);}
bool VekScriptEngine::LoadSource(const std::string& source,const std::string& name){
    try{if(source.size()>impl->policy.maxSourceBytes)throw std::runtime_error("VEK security: source exceeds configured size limit");Lexer l(source,impl->policy);auto tokens=l.Scan();Parser p(std::move(tokens),impl->policy);impl->functions=p.ParseProgram();impl->sourceName=name;impl->lastError.clear();impl->loaded=true;return true;}
    catch(const std::exception& e){impl->functions.clear();impl->sourceName=name;impl->lastError=e.what();impl->loaded=false;return false;}}
void VekScriptEngine::Clear(){impl->functions.clear();impl->lastError.clear();impl->sourceName.clear();impl->loaded=false;}
void VekScriptEngine::SetSecurityPolicy(const VekSecurityPolicy& p){impl->policy=p;}
const VekSecurityPolicy& VekScriptEngine::GetSecurityPolicy() const{return impl->policy;}
bool VekScriptEngine::RegisterNative(const std::string& n,NativeFunction f){if(impl->nativesSealed){impl->lastError="VEK security: native registry is sealed";return false;}impl->natives[n]=std::move(f);return true;}
void VekScriptEngine::SealNativeRegistry(){impl->nativesSealed=true;}
bool VekScriptEngine::NativeRegistrySealed() const{return impl->nativesSealed;}
bool VekScriptEngine::HasFunction(const std::string& n) const{return impl->functions.find(n)!=impl->functions.end();}
VekValue VekScriptEngine::Call(const std::string& n,const std::vector<VekValue>& args){
    try{impl->lastError.clear();if(args.size()>impl->policy.maxArgumentsPerCall)throw std::runtime_error("VEK security: top-level argument limit exceeded");impl->instructionsRemaining=impl->policy.maxInstructionsPerCall;impl->nativeCallsRemaining=impl->policy.maxNativeCallsPerCall;impl->loopIterationsRemaining=impl->policy.maxLoopIterationsPerCall;impl->callDepth=0;return impl->CallFunction(n,args);}
    catch(const std::exception& e){impl->lastError=e.what();return {};}}
bool VekScriptEngine::IsLoaded() const{return impl->loaded;}
const std::string& VekScriptEngine::LastError() const{return impl->lastError;}
const std::string& VekScriptEngine::SourceName() const{return impl->sourceName;}

void VekRegisterStandardLibrary(VekScriptEngine& e){
    e.RegisterNative("print",[](const std::vector<VekValue>& a){for(const auto& v:a)std::cout<<v.AsString();return VekValue();});
    e.RegisterNative("println",[](const std::vector<VekValue>& a){for(std::size_t i=0;i<a.size();++i){if(i)std::cout<<" ";std::cout<<a[i].AsString();}std::cout<<"\n";return VekValue();});
    e.RegisterNative("type",[](const std::vector<VekValue>& a){if(a.empty())return VekValue("nil");if(a[0].IsNumber())return VekValue("number");if(a[0].IsBool())return VekValue("bool");if(a[0].IsString())return VekValue("string");return VekValue("nil");});
    e.RegisterNative("len",[](const std::vector<VekValue>& a){return VekValue(a.empty()?0.0:static_cast<double>(a[0].AsString().size()));});
    e.RegisterNative("number",[](const std::vector<VekValue>& a){return VekValue(a.empty()?0.0:a[0].AsNumber());});
    e.RegisterNative("string",[](const std::vector<VekValue>& a){return VekValue(a.empty()?std::string("nil"):a[0].AsString());});
    e.RegisterNative("abs",[](const std::vector<VekValue>& a){return VekValue(std::fabs(a.at(0).AsNumber()));});
    e.RegisterNative("floor",[](const std::vector<VekValue>& a){return VekValue(std::floor(a.at(0).AsNumber()));});
    e.RegisterNative("ceil",[](const std::vector<VekValue>& a){return VekValue(std::ceil(a.at(0).AsNumber()));});
    e.RegisterNative("round",[](const std::vector<VekValue>& a){return VekValue(std::round(a.at(0).AsNumber()));});
    e.RegisterNative("sqrt",[](const std::vector<VekValue>& a){return VekValue(std::sqrt(std::max(0.0,a.at(0).AsNumber())));});
    e.RegisterNative("pow",[](const std::vector<VekValue>& a){return VekValue(std::pow(a.at(0).AsNumber(),a.at(1).AsNumber()));});
    e.RegisterNative("min",[](const std::vector<VekValue>& a){return VekValue(std::min(a.at(0).AsNumber(),a.at(1).AsNumber()));});
    e.RegisterNative("max",[](const std::vector<VekValue>& a){return VekValue(std::max(a.at(0).AsNumber(),a.at(1).AsNumber()));});
    e.RegisterNative("clamp",[](const std::vector<VekValue>& a){return VekValue(std::clamp(a.at(0).AsNumber(),a.at(1).AsNumber(),a.at(2).AsNumber()));});
}
