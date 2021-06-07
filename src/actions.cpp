namespace sx {

// accounts to be notified via inline action
void curve::notify()
{
    curve::config_table _config( get_self(), get_self().value );
    auto config = _config.get_or_default();

    for ( const name notifier : config.notifiers ) {
        if ( is_account( notifier ) ) require_recipient( notifier );
    }
}

[[eosio::action]]
void curve::liquiditylog( const symbol_code pair_id, const name owner, const name action, const asset liquidity, const asset quantity0,  const asset quantity1, const asset total_liquidity, const asset reserve0, const asset reserve1 )
{
    require_auth( get_self() );
    notify();
    require_recipient( owner );
}

[[eosio::action]]
void curve::swaplog( const symbol_code pair_id, const name owner, const name action, const asset quantity_in, const asset quantity_out, const asset fee, const double trade_price, const asset reserve0, const asset reserve1 )
{
    require_auth( get_self() );
    notify();
    require_recipient( owner );
}

void curve::create( const extended_symbol value )
{
    eosio::token::create_action create( value.get_contract(), { value.get_contract(), "active"_n });
    create.send( get_self(), asset{ asset_max, value.get_symbol() } );
}

void curve::issue( const extended_asset value, const string memo )
{
    eosio::token::issue_action issue( value.contract, { get_self(), "active"_n });
    issue.send( get_self(), value.quantity, memo );
}

void curve::retire( const extended_asset value, const string memo )
{
    eosio::token::retire_action retire( value.contract, { get_self(), "active"_n });
    retire.send( value.quantity, memo );
}

void curve::transfer( const name from, const name to, const extended_asset value, const string memo )
{
    eosio::token::transfer_action transfer( value.contract, { from, "active"_n });
    transfer.send( from, to, value.quantity, memo );
}

} // namespace sx