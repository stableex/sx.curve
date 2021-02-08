void sx::curve::update_amplifiers( )
{
    sx::curve::ramp_table _ramp( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // update `pairs` table with active ramp up/down that are different
    for ( const auto ramp : _ramp ) {
        auto pair_itr = _pairs.find(ramp.pair_id.raw());
        if ( pair_itr == _pairs.end() ) continue;
        const uint64_t amplifier = sx::curve::get_amplifier( ramp.pair_id );

        // modify pairs table if amplifier is different
        if ( pair_itr->amplifier != amplifier ) {
            _pairs.modify( pair_itr, get_self(), [&]( auto & row ) {
                row.amplifier = amplifier;
                row.last_updated = current_time_point();
            });
        }
    }
}