void sx::curve::convert(const extended_asset ext_in, const extended_asset ext_min_out, name receiver)
{
    // find all possible trade paths
    auto paths = find_trade_paths( ext_in.quantity.symbol.code(), ext_min_out.quantity.symbol.code() );
    check(paths.size(), "Curve.sx: no path for exchange");

    // choose trade path that gets the best return
    auto best_path = paths[0];
    extended_asset best_out;
    for (const auto& path: paths) {
        auto out = apply_trade(ext_in, path);
        if (out.quantity.amount > best_out.quantity.amount) {
            best_path = path;
            best_out = out;
        }
    }
    check(best_out.quantity.amount, "Curve.sx: no matching exchange");
    check(ext_min_out.contract.value == 0 || ext_min_out.contract == best_out.contract, "Curve.sx: reserve_out vs memo contract mismatch");
    check(ext_min_out.quantity.amount == 0 || ext_min_out.quantity.symbol == best_out.quantity.symbol, "Curve.sx: return vs memo symbol precision mismatch");
    check(ext_min_out.quantity.amount == 0 || ext_min_out.quantity.amount <= best_out.quantity.amount, "Curve.sx: return is not enough");

    // execute the trade by updating all involved pools
    best_out = apply_trade(ext_in, best_path, true);

    // transfer amount to receiver
    transfer( get_self(), receiver, best_out, "Curve.sx: swap token" );
}

// find all possible paths to exchange symcode_in to memo symcode, include 2-hops
vector<vector<symbol_code>> sx::curve::find_trade_paths( const symbol_code symcode_in, const symbol_code symcode_out )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    check( symcode_in != symcode_out, "Curve.sx: onversion target symbol must not match incoming symbol");

    // find first valid hop
    vector<vector<symbol_code>> paths;
    vector<pair<symbol_code, symbol_code>> hop_one;     // {pair id, first hop symbol_code}
    for (const auto& row : _pairs) {
        symbol_code sc1 = row.reserve0.quantity.symbol.code();
        symbol_code sc2 = row.reserve1.quantity.symbol.code();

        if (sc1 != symcode_in) std::swap(sc1, sc2);
        if (sc1 != symcode_in) continue;
        if (sc2 == symcode_out) paths.push_back({row.id});  //direct path
        else hop_one.push_back({row.id, sc2});
    }

    // find all possible second hops
    for(const auto& [id1, sc_in] : hop_one) {
        auto id2 = find_pair_id(sc_in, symcode_out);
        if(id2.is_valid()) paths.push_back({id1, id2});
    }

    return paths;
}

// find pair_id based on symbol_code of incoming and target tokens
symbol_code sx::curve::find_pair_id( const symbol_code symcode_in, const symbol_code symcode_out )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // find by combination of input quantity & memo symbol
    auto _pairs_by_reserves = _pairs.get_index<"byreserves"_n>();
    auto itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_in, symcode_out ) );
    if ( itr != _pairs_by_reserves.end() ) return itr->id;

    itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_out, symcode_in ) );
    if ( itr != _pairs_by_reserves.end() ) return itr->id;

    // nothing found - return empty
    return {};
}
