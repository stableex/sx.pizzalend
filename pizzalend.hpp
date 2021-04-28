#pragma once

#include <eosio/asset.hpp>
#include <sx.utils/utils.hpp>

namespace pizzalend {

    using namespace eosio;
    using namespace sx;

    const name id = "pizzalend"_n;
    const name code = "lend.pizza"_n;
    const std::string description = "Lend.Pizza Converter";
    const name token_code = "pztken.pizza"_n;

    struct pztoken_config {
        asset           base_rate;
        asset           max_rate;
        asset           base_discount_rate;
        asset           max_discount_rate;
        asset           best_usage_rate;
        asset           floating_fee_rate;
        asset           fixed_fee_rate;
        asset           liqdt_rate;
        asset           liqdt_bonus;
        asset           max_ltv;
        asset           floating_rate_power;
        bool            is_collateral;
        bool            can_stable_borrow;
        uint8_t         borrow_liqdt_order;
        uint8_t         collateral_liqdt_order;
    };

    struct [[eosio::table]] pztoken_row {
        name            pztoken;
        extended_symbol pzsymbol;
        extended_symbol anchor;
        asset           cumulative_deposit;
        asset           available_deposit;
        asset           pzquantity;
        asset           borrow;
        asset           cumulative_borrow;
        asset           variable_borrow;
        asset           stable_borrow;
        asset           usage_rate;
        asset           floating_rate;
        asset           discount_rate;
        asset           price;
        double_t        pzprice;
        double_t        pzprice_rate;
        uint64_t        updated_at;
        pztoken_config  config;

        uint64_t primary_key() const { return pztoken.value; }
        uint128_t get_secondary1() const { return 0; }          //unknown
        uint128_t get_by_anchor() const { return static_cast<uint128_t>(anchor.get_contract().value) << 8 | anchor.get_symbol().code().raw(); }
        uint64_t get_by_borrowliqorder() const { return config.borrow_liqdt_order; }
        uint64_t get_by_collatliqorder() const { return config.collateral_liqdt_order; }
    };
    typedef eosio::multi_index< "pztoken"_n, pztoken_row > pztoken;

    static bool is_pztoken( const symbol& sym ) {
        return utils::get_supply({ sym, token_code }).symbol.is_valid();
    }

    static extended_symbol get_pztoken( const symbol_code& symcode) {
        pztoken pztoken_tbl( code, code.value);
        for(const auto& row: pztoken_tbl) {
            if(row.anchor.get_symbol().code() == symcode) {
                return row.pzsymbol;
            }
        }
        return {};
    }

    static extended_asset wrap( const asset& quantity ) {

        pztoken pztoken_tbl(code, code.value);
        for(const auto& row: pztoken_tbl) {
            if(row.anchor.get_symbol() == quantity.symbol) {
                return { static_cast<int64_t>( quantity.amount / row.pzprice ), row.pzsymbol };
            }
        }
        check(false, "pizzalend: not lendable: " + quantity.to_string());
        return { };
    }

    static extended_asset unwrap( const asset& pzqty ) {
        pztoken pztoken_tbl(code, code.value);
        for(const auto& row: pztoken_tbl) {
            if(row.pzsymbol.get_symbol() == pzqty.symbol) {
                int64_t amount_out = row.pzprice * pzqty.amount;
                if(amount_out > row.available_deposit.amount) amount_out = 0;
                return { amount_out, row.anchor };
            }
        }
        check(false, "pizzalend: not redeemable: " + pzqty.to_string());
        return { };
    }
    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair id, returns the calculated return
     *
     * ### params
     *
     * - `{asset} in` - input amount
     * - `{symbol} out_sym` - out symbol
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const asset in = asset { 10000, "USDT" };
     * const symbol out_sym = symbol { "PZUSDT,4" };
     *
     * // Calculation
     * const asset out = pizzalend::get_amount_out( in, out_sym );
     * // => 0.999612 BUSDT
     * ```
     */
    static asset get_amount_out( const asset quantity, const symbol out_sym )
    {
        if(is_pztoken(out_sym)) {
            const auto out = wrap(quantity).quantity;
            if(out.symbol == out_sym) return out;
        }

        if(is_pztoken(quantity.symbol)) {
            const auto out = unwrap(quantity).quantity;
            if(out.symbol == out_sym) return out;
        }

        check(false, "sx.pizzalend: Not B-token");
        return {};
    }

}