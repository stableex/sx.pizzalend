#pragma once

#include <eosio/asset.hpp>
#include <sx.utils/utils.hpp>

namespace pizzalend {

    using namespace eosio;
    using namespace sx;
    using namespace sx::utils;

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
        uint8_t type;           //1 = variable rate, 2 = fixed rate
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

    static pztoken_row get_reserve( const extended_symbol& anchor_ext_sym) {
        pztoken pztoken_tbl( code, code.value);
        for(const auto& row: pztoken_tbl) {
            if(row.anchor == anchor_ext_sym) {
                return row;
            }
        }
        check(false, "pizzalend::get_reserve(): anchor doesn't exist");
        return {};
    }

    static extended_symbol get_anchor( const name pzname) {
        pztoken pztoken_tbl( code, code.value);
        const auto it = pztoken_tbl.find( pzname.value );
        return it == pztoken_tbl.end() ? extended_symbol{} : it->anchor;
    }

    static vector<liqdtorder_row> get_liq_accounts( const double min_value ){
        liqdtorder liqdtordertbl( code, code.value );
        vector<liqdtorder_row> res;
        for(const auto& row: liqdtordertbl) {
            const auto loan_res = get_reserve( row.loan.get_extended_symbol() );
            const double loan_price = loan_res.price.amount / pow(10, loan_res.price.symbol.precision());
            const double liq_value = row.loan.quantity.amount / pow(10, row.loan.quantity.symbol.precision()) * loan_price;
            if(liq_value > min_value) res.push_back(row);
        }
        return res;
    }
    static liqdtorder_row get_auction( const uint64_t id ){
        liqdtorder liqdtordertbl( code, code.value );
        return liqdtordertbl.get(id, "pizzalend: can't find auction");
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

        check(false, "sx.pizzalend: Not pz-token");
        return {};
    }

    /**
     * ## STATIC `get_oraclized_value`
     *
     * Given tokens return oraclized value and ratioed liquidation value
     *
     * ### params
     *
     * - `{extended_asset} tokens` - tokens, i.e. "1000.0000 USDT@tethertether"
     * - `{name} pzname` - pz name, i.e. "pzusdt"_n
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const asset tokens = "1000.0000 USDT@tethertether"
     * const name pzname = "pzusdt";
     *
     * // Calculation
     * const double [ value, liq_value ] = pizzalend::get_oraclized_value( tokens, pzname );
     * // => [ 1000.0, 800.0]
     * ```
     */
    static pair<double, double> get_oraclized_value( const extended_asset ext_tokens, const name pzname )
    {
        collateral_table _table( code, code.value);
        pztoken pztoken_tbl( code, code.value);
        const auto row = pztoken_tbl.get(pzname.value, "pizzalend: get_oraclized_value(): invalid pzname");
        check(row.anchor == ext_tokens.get_extended_symbol(), "pizzalend: get_oraclized_value(): invalid pzname/asset combo" );

        const double price = row.price.amount / pow(10, row.price.symbol.precision());
        const double token_value = ext_tokens.quantity.amount / pow(10, ext_tokens.quantity.symbol.precision()) * price;
        const double liq_rate = row.config.liqdt_rate.amount / pow(10, row.config.liqdt_rate.symbol.precision());

        return { token_value, token_value * liq_rate };
    }

    /**
     * ## STATIC `get_collaterals`
     *
     * Given an account name return collaterals and their values
     *
     * ### params
     *
     * - `{name} account` - account
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const name account = "myusername";
     *
     * // Calculation
     * const vector<StOraclizedAsset> collaterals = pizzalend::get_collaterals( account );
     * // => { {"400 USDT", 400, 300}, {"100 EOS", 500, 375} }
     * ```
     */
    static vector<OraclizedAsset> get_collaterals( const name account )
    {
        vector<OraclizedAsset> res;
        collateral_table _table( code, code.value);
        auto index = _table.get_index<"byaccount"_n>();
        auto it = index.lower_bound(account.value);
        while( it != index.end() ) {
            if( it->account != account) break;
            const auto pztokens = it->quantity;
            const auto ext_tokens = unwrap(pztokens);
            const auto [ value, ratioed_value ] = get_oraclized_value(ext_tokens, it->pzname);
            res.push_back({ ext_tokens, value, ratioed_value });
            ++it;
        }

        return res;
    }

        /**
     * ## STATIC `get_loans`
     *
     * Given an account name return loans and their values
     *
     * ### params
     *
     * - `{name} account` - account
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const name account = "myusername";
     *
     * // Calculation
     * const vector<StOraclizedAsset> collaterals = pizzalend::get_loans( account );
     * // => { {"400 USDT", 400, 300}, {"100 EOS", 500, 375} }
     * ```
     */
    static vector<OraclizedAsset> get_loans( const name account )
    {
        vector<OraclizedAsset> res;
        loan_table _table( code, code.value);
        auto index = _table.get_index<"byaccount"_n>();
        auto it = index.lower_bound(account.value);
        while( it != index.end() ) {
            if( it->account != account) break;
            // TODO: take interest accrued since last update into account
            // using precision x10000, so we adjust and round up
            const auto tokens = asset{ it->quantity.amount / 10000 + 1, it->principal.symbol };
            const auto ext_sym = get_anchor(it->pzname);
            const extended_asset ext_tokens = { tokens, ext_sym.get_contract() };
            const auto [ value, ratioed_value ] = get_oraclized_value(ext_tokens, it->pzname);
            res.push_back({ ext_tokens, value, value });
            ++it;
        }

        return res;
    }

    /**
     * ## STATIC `get_health_factor`
     *
     * Given an loans and collaterals return health factor
     *
     * ### params
     *
     * - `{vector<OraclizedAsset>} loans` - loans
     * - `{vector<OraclizedAsset>} collaterals` - collaterals
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const vector<OraclizedAsset> loans = { {"400 USDT", 400, 400}, {"100 EOS", 500, 500} };
     * const vector<OraclizedAsset> collaterals = { {"500 USDT", 700, 300}, {"200 EOS", 600, 375} };
     *
     * // Calculation
     * const double health_factor = pizzalend::get_health_factor( loans, collaterals );
     * // => 1.2345
     * ```
     */
    static double get_health_factor( const vector<OraclizedAsset>& loans, const vector<OraclizedAsset>& collaterals )
    {
        double deposited = 0, loaned = 0;
        for(const auto coll: collaterals){
            deposited += coll.ratioed;
        }

        for(const auto loan: loans){
            loaned += loan.value;
        }
        return loaned == 0 ? 0 : deposited / loaned;
    }


    /**
     * ## STATIC `get_health_factor`
     *
     * Given an account name return user health factor
     *
     * ### params
     *
     * - `{name} account` - account
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const name account = "myusername";
     *
     * // Calculation
     * const double health_factor = pizzalend::get_health_factor( account );
     * // => 1.2345
     * ```
     */
    static double get_health_factor( const name account )
    {
        const auto collaterals = pizzalend::get_collaterals(account);
        const auto loans = pizzalend::get_loans(account);

        return pizzalend::get_health_factor(loans, collaterals);
    }

    /**
     * ## STATIC `get_liquidation_out`
     *
     * Calculate collateral tokens to receive when liquidating {ext_in} debt
     *
     * ### params
     *
     * - `{extended_asset} ext_in` - amount of debt to liquidate
     * - `{extended_symbol} ext_sym_out` - desired collateral to receive
     * - `{vector<OraclizedAsset>} loans` - user loans
     * - `{vector<OraclizedAsset>} collaterals` - user collaterals
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const extended_asset ext_in = { "400 USDT@tethertether" };
     * const extended_symbol ext_sym_out = { "4,EOS@eosio.token" };
     * const vector<OraclizedAsset> loans = { {"400 USDT", 400, 400}, {"100 EOS", 500, 500} };
     * const vector<OraclizedAsset> collaterals = { {"500 USDT", 700, 300}, {"200 EOS", 600, 375} };
     *
     * // Calculation
     * const auto out = pizzalend::get_liquidation_out( ext_in, ext_sym_out, loans, collaterals );
     * // => 100 EOS
     * ```
     */
    static extended_asset get_liquidation_out( const extended_asset ext_in, const extended_symbol ext_sym_out, const vector<OraclizedAsset>& loans, const vector<OraclizedAsset>& collaterals )
    {
        // no need to check health factor - if we are in the table then it's < 1
        // const auto hf = get_health_factor(loans, collaterals);
        // if(hf >= 1) return { 0, ext_sym_out };

        double loans_value = 0;
        extended_asset loan_to_liquidate, coll_to_get;
        for(const auto loan: loans){
            if(loan.tokens.get_extended_symbol() == ext_in.get_extended_symbol() ) loan_to_liquidate = loan.tokens;
            loans_value += loan.value;
        }
        for(const auto coll: collaterals){
            if(coll.tokens.get_extended_symbol() == ext_sym_out ) coll_to_get = coll.tokens;
        }
        if(loan_to_liquidate.quantity.amount == 0 || loan_to_liquidate < ext_in || coll_to_get.quantity.amount == 0)
            return { 0, ext_sym_out };

        const auto loan_res = get_reserve( ext_in.get_extended_symbol() );
        const auto coll_res = get_reserve( ext_sym_out );
        const double bonus = coll_res.config.liqdt_bonus.amount / pow(10, coll_res.config.liqdt_bonus.symbol.precision());
        const double loan_price = loan_res.price.amount / pow(10, loan_res.price.symbol.precision());
        const double coll_price = coll_res.price.amount / pow(10, coll_res.price.symbol.precision());
        const double liq_value = ext_in.quantity.amount / pow(10, ext_in.quantity.symbol.precision()) * loan_price;
        const double value_out = liq_value * (1 + bonus * 2/3);     // receiving 2/3 of the bonus
        const int64_t out = value_out / coll_price * pow(10, coll_res.anchor.get_symbol().precision());

        // print("\n  In: ", ext_in.quantity, " loan_price: ", loan_price, " coll_price: ", coll_price, " liq_value: ", liq_value, " value_out: ", value_out, " out: ", out);
        if(liq_value > loans_value) return { 0, ext_sym_out };
        if(coll_to_get.quantity.amount < out) return { 0, ext_sym_out };   //can't get more than collateral

        return { out, ext_sym_out };
    }

}