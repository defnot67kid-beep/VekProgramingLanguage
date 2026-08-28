# VEK 1.0 Language Reference

## Values
`nil`, numbers (double precision), booleans, strings.

## Variables
```vek
let speed = 25;
speed = speed + 5;
```
Variables are function-local in VEK 1.0.

## Functions
```vek
fn add(a, b) { return a + b; }
```

## Branches
```vek
if speed > 100 { return "fast"; }
else if speed > 40 { return "normal"; }
else { return "slow"; }
```

## Loops
```vek
let i = 0;
while i < 10 {
    i = i + 1;
    if i == 3 { continue; }
    if i == 8 { break; }
}
```

## Operators
Precedence, high to low: unary `! -`; `* / %`; `+ -`; comparisons; equality; `&&`; `||`.

## Comments
`# comment` and `// comment`.

## Standard library (CLI)
`print`, `println`, `type`, `len`, `number`, `string`, `abs`, `floor`, `ceil`, `round`, `sqrt`, `pow`, `min`, `max`, `clamp`.

Embedded hosts choose which natives to register and can seal the registry.
