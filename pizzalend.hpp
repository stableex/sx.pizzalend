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

    static const map<symbol_code, name> PZ_KEYS= {
        { symbol_code{"PZUSDT"}, "pzusdt"_n },
        { symbol_code{"USDT"}, "pzusdt"_n },
        { symbol_code{"PZUSN"}, "pzusn"_n },
        { symbol_code{"USN"}, "pzusn"_n },
        { symbol_code{"PZOUSD"}, "pzousd"_n },
        { symbol_code{"OUSD"}, "pzousd"_n }
    };

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
        uint128_t get_secondary1() const { return 0; }          // unknown
        uint128_t get_by_anchor() const { return static_cast<uint128_t>(anchor.get_contract().value) << 64 | anchor.get_symbol().code().raw(); }     // wrong
        uint64_t get_by_borrowliqorder() const { return config.borrow_liqdt_order; }
        uint64_t get_by_collatliqorder() const { return config.collateral_liqdt_order; }
    };
    typedef eosio::multi_index< "pztoken"_n, pztoken_row > pztoken;

    struct [[eosio::table]] collateral_row {
        uint64_t id;
        name account;
        name pzname;
        asset quantity;
        uint64_t updated_at;

        uint64_t primary_key() const { return id; }
        uint64_t get_by_account() const { return account.value; }
        uint64_t get_by_pzname() const { return pzname.value; }
        uint128_t get_by_accpzname() const { return static_cast<uint128_t>(account.value) << 64 | pzname.value; }
    };
    typedef eosio::multi_index< "collateral"_n, collateral_row,
        indexed_by< "byaccount"_n, const_mem_fun<collateral_row, uint64_t, &collateral_row::get_by_account> >,
        indexed_by< "bypzname"_n, const_mem_fun<collateral_row, uint64_t, &collateral_row::get_by_pzname> >,
        indexed_by< "byaccpzname"_n, const_mem_fun<collateral_row, uint128_t, &collateral_row::get_by_accpzname> >
    > collateral_table;

    struct [[eosio::table]] loan_row {
        uint64_t id;
        name account;
        name pzname;
        asset principal;
        asset quantity;
        uint8_t type;
        asset fixed_rate;
        uint64_t turn_variable_countdown;
        uint64_t last_calculated_at;
        uint64_t updated_at;

        uint64_t primary_key() const { return id; }
        uint64_t get_by_account() const { return account.value; }
        uint64_t get_by_pzname() const { return pzname.value; }
        uint128_t get_by_accpzname() const { return static_cast<uint128_t>(account.value) << 64 | pzname.value; }
    };
    typedef eosio::multi_index< "loan"_n, loan_row,
        indexed_by< "byaccount"_n, const_mem_fun<loan_row, uint64_t, &loan_row::get_by_account> >,
        indexed_by< "bypzname"_n, const_mem_fun<loan_row, uint64_t, &loan_row::get_by_pzname> >,
        indexed_by< "byaccpzname"_n, const_mem_fun<loan_row, uint128_t, &loan_row::get_by_accpzname> >
    > loan_table;

    struct [[eosio::table]] cachedhealth_row {
        name account;
        double_t loan_value;
        double_t collateral_value;
        double_t factor;
        uint64_t updated_at;

        uint64_t primary_key() const { return account.value; }
    };
    typedef eosio::multi_index< "cachedhealth"_n, cachedhealth_row > cachedhealth_table;

    struct [[eosio::table]] liqdtorder_row {
        uint64_t        id;
        name            account;
        extended_asset  collateral;
        extended_asset  loan;
        uint64_t        liqdted_at;
        uint64_t        updated_at;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "liqdtorder"_n, liqdtorder_row > liqdtorder;

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

        // if we know pztoken name - use primary key for speed
        // TODO: figure out secondary key to pull by anchor (?)
        if( PZ_KEYS.count(quantity.symbol.code()) ){
            const auto& row = pztoken_tbl.get( PZ_KEYS.at(quantity.symbol.code()).value, "pizzalend: not lendable");
            return { static_cast<int64_t>( quantity.amount / row.pzprice ), row.pzsymbol };
        }

        // otherwise - just iterate
        for(const auto& row: pztoken_tbl) {
            if(row.anchor.get_symbol() == quantity.symbol) {
                return { static_cast<int64_t>( quantity.amount / row.pzprice ), row.pzsymbol };
            }
        }
        check(false, "pizzalend: not lendable: " + quantity.to_string());
        return { };
    }

    static extended_asset unwrap( const asset& pzqty, bool ignore_deposit = false) {
        pztoken pztoken_tbl(code, code.value);

        // if we know pztoken name - use primary key for speed
        // TODO: figure out secondary key to pull by pztoken (?)
        if( PZ_KEYS.count(pzqty.symbol.code()) ){
            const auto& row = pztoken_tbl.get( PZ_KEYS.at(pzqty.symbol.code()).value, "pizzalend: not redeemable");
            int64_t amount_out = row.pzprice * pzqty.amount;
            if(amount_out > row.available_deposit.amount && !ignore_deposit) amount_out = 0;
            return { amount_out, row.anchor };
        }

        // otherwise - just iterate
        for(const auto& row: pztoken_tbl) {
            if(row.pzsymbol.get_symbol() == pzqty.symbol) {
                int64_t amount_out = row.pzprice * pzqty.amount;
                if(amount_out > row.available_deposit.amount && !ignore_deposit) amount_out = 0;
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