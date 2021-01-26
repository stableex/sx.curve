[[eosio::action]]
void sx::curve::test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee )
{
    print ("curve::get_amount_out(",amount,"): ", Curve::get_amount_out( amount, reserve_in, reserve_out, amplifier, fee ), "\n");

    check(false, "see print");
}

[[eosio::action]]
void sx::curve::reset()
{
    require_auth( get_self() );

    sx::curve::config _config( get_self(), get_self().value );
    sx::curve::pairs _pairs( get_self(), get_self().value );

    _config.remove();
    clear_table( _pairs );
}

template <typename T>
void sx::curve::clear_table( T& table )
{
    auto itr = table.begin();
    while ( itr != table.end() ) {
        itr = table.erase( itr );
    }
}

[[eosio::action]]
void sx::curve::backup()
{
    sx::curve::pairs _pairs( get_self(), get_self().value );
    sx::curve::backup_table _backup( get_self(), get_self().value );
    clear_table( _backup );

    auto itr = _pairs.begin();
    while ( itr != _pairs.end() ) {
        _backup.emplace( get_self(), [&]( auto & row ) {
            row.id = itr->id;
            row.reserve0 = itr->reserve0;
            row.reserve1 = itr->reserve1;
            row.liquidity = itr->liquidity;
            row.amplifier = itr->amplifier;
            row.price0_last = itr->price0_last;
            row.price1_last = itr->price1_last;
            // row.volume0 = itr->volume0;
            // row.volume1 = itr->volume1;
            row.last_updated = itr->last_updated;
        });
        ++itr;
    }
}

[[eosio::action]]
void sx::curve::copy()
{
    sx::curve::pairs _pairs( get_self(), get_self().value );
    sx::curve::backup_table _backup( get_self(), get_self().value );
    clear_table( _pairs );

    auto itr = _backup.begin();
    while ( itr != _backup.end() ) {
        _pairs.emplace( get_self(), [&]( auto & row ) {
            row.id = itr->id;
            row.reserve0 = itr->reserve0;
            row.reserve1 = itr->reserve1;
            row.liquidity = itr->liquidity;
            row.amplifier = itr->amplifier;
            row.virtual_price = calculate_virtual_price( itr->reserve0.quantity, itr->reserve1.quantity, itr->liquidity.quantity );
            row.price0_last = itr->price0_last;
            row.price1_last = itr->price1_last;
            row.volume0 = { int64_t(itr->volume0), itr->reserve0.quantity.symbol };
            row.volume1 = { int64_t(itr->volume1), itr->reserve1.quantity.symbol } ;
            row.last_updated = itr->last_updated;
        });
        ++itr;
    }
}