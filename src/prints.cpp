void sx::curve::withdraw_liquidity( const name owner, const extended_asset value )
{
    print( "\nexisting supply: ", supply, "\n");
    print( "existing reserve0: ", reserve0, "\n");
    print( "existing reserve1: ", reserve1, "\n");
    print( "reserve_ratio0: ", reserve_ratio0, "\n");
    print( "reserve_ratio1: ", reserve_ratio1, "\n");
    print( "reserves: ", reserves, "\n");
    print( "payment: ", payment, "\n");
    print( "retire_amount: ", retire_amount, "\n");
    print( "outgoing amount0: ", amount0, "\n");
    print( "outgoing amount1: ", amount1, "\n");
    print( "outgoing out0: ", out0, "\n");
    print( "outgoing out1: ", out1, "\n");

    print( "\nDepositing: ", value);
    print( "\nWithdrawing ", out0, " + ", out1, " to ", owner);
}

[[eosio::action]]
void sx::curve::deposit( const name owner, const symbol_code pair_id )
{
    print( "\nexisting supply: " + to_string(supply) + "\n");
    print( "existing reserve0: " + to_string(reserve0) + "\n");
    print( "existing reserve1: " + to_string(reserve1) + "\n");
    print( "reserve_ratio0: " + to_string(reserve_ratio0) + "\n");
    print( "reserve_ratio1: " + to_string(reserve_ratio1) + "\n");
    print( "reserves: " + to_string(reserves) + "\n");
    print( "payment: " + to_string(payment) + "\n");
    print( "amount_ratio0: " + to_string(amount_ratio0) + "\n");
    print( "amount_ratio1: " + to_string(amount_ratio1) + "\n");
    print( "incoming amount0: " + to_string(amount0) + "\n");
    print( "incoming amount1: " + to_string(amount1) + "\n");

    print("\nSending excess ", excess, " to ", owner);

    print( "\nDepositing: ", ext_deposit0, " + ", ext_deposit1);
    print( "\nIssuing ", issued, " and sending to ", owner);
}