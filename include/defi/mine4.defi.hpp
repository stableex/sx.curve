#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

namespace defi {
    class [[eosio::contract("mine4.defi")]] mine4 : public eosio::contract {
    public:
        using contract::contract;

        struct [[eosio::table("miners1")]] miners1_row {
            uint64_t            pool_id;
            asset               unclaimed;
            asset               claimed;
            uint64_t            debt;

            uint64_t primary_key() const { return pool_id; }
        };
        typedef eosio::multi_index< "miners1"_n, miners1_row> miners1_table;

        [[eosio::action]]
        void claimall( const name owner );

        using claimall_action = eosio::action_wrapper<"claimall"_n, &defi::mine4::claimall>;
    };
}