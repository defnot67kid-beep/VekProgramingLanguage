#include <vek/VekScriptEngine.h>
#include <vek/VekGameSystems.h>
#include <iostream>
#include <string>

static int runFile(const std::string& path) {
    VekScriptEngine vm; VekRegisterStandardLibrary(vm); vek::VekRegisterGameplayLibrary(vm);
    if (!vm.LoadFile(path)) { std::cerr << vm.LastError() << "\n"; return 2; }
    if (!vm.HasFunction("main")) { std::cerr << "VEK: program has no fn main()\n"; return 3; }
    VekValue result = vm.Call("main");
    if (!vm.LastError().empty()) { std::cerr << vm.LastError() << "\n"; return 4; }
    if (!result.IsNil()) std::cout << result.AsString() << "\n";
    return 0;
}

static int checkFile(const std::string& path) {
    VekScriptEngine vm;
    if (!vm.LoadFile(path)) { std::cerr << vm.LastError() << "\n"; return 2; }
    std::cout << "OK: " << path << "\n"; return 0;
}

static int evalExpr(const std::string& expr) {
    VekScriptEngine vm; VekRegisterStandardLibrary(vm); vek::VekRegisterGameplayLibrary(vm);
    std::string src = "fn main(){ return " + expr + "; }";
    if (!vm.LoadSource(src, "<eval>")) { std::cerr << vm.LastError() << "\n"; return 2; }
    auto v=vm.Call("main"); if(!vm.LastError().empty()){std::cerr<<vm.LastError()<<"\n";return 3;} std::cout<<v.AsString()<<"\n"; return 0;
}

static int repl() {
    std::cout << "VEK " << VEK_VERSION_STRING << " REPL. :quit to exit. Expressions only.\n";
    std::string line;
    while (true) {
        std::cout << "vek> ";
        if (!std::getline(std::cin,line) || line==":quit" || line==":q") break;
        if (line.empty()) continue;
        evalExpr(line);
    }
    return 0;
}

int main(int argc,char** argv){
    if(argc<2){std::cout<<"VEK "<<VEK_VERSION_STRING<<"\nUsage: vek <run|check|eval|repl|version> [file/expression]\n";return 0;}
    std::string cmd=argv[1];
    if(cmd=="version"||cmd=="--version"){std::cout<<"VEK "<<VEK_VERSION_STRING<<"\n";return 0;}
    if(cmd=="repl")return repl();
    if(cmd=="run"&&argc>=3)return runFile(argv[2]);
    if(cmd=="check"&&argc>=3)return checkFile(argv[2]);
    if(cmd=="eval"&&argc>=3)return evalExpr(argv[2]);
    std::cerr<<"Invalid VEK command.\n";return 1;
}
