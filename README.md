# **`SX.PizzaLend`**

> C++ intetrface for interacting with `pizza.lend` smart contract


## Quickstart

```c++
#include <sx.pizzalend/pizzalend.hpp>

const asset in = asset { 100000, "USDT" };
const symbol out_sym = symbol { "BUSDT,4" };

// calculate out price
const asset out = pizzalend::get_amount_out( in, out_sym);
// => "8.6500 BUSDT"
```

```c++
const asset in = asset { 100000, "USDT" };

// calculate wrapped amount
const asset btokens = pizzalend::wrap( in );
// => "8.6500 BUSDT"
```

```c++
const asset in = asset { 100000, "BUSDT" };

// calculate unwrapped amount
const asset tokens = pizzalend::unwrap( in );
// => "11.4500 USDT"
```
