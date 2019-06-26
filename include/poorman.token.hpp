/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

using namespace eosio;

namespace genereos {

    using std::string;

    class token : public contract {
        public:
            using contract::contract;

            // Constructor
            token( name receiver, name code, datastream<const char *> ds ):
                contract( receiver, code, ds ) {}
            
            // Genereos Token Contract
            ACTION create( name  issuer,
                           asset maximum_supply );

            ACTION issue(        name to, asset quantity, string memo );
            ACTION issuefree(    name to, asset quantity, string memo );
            ACTION burn(         name from, asset quantity, string memo );
            ACTION signup(       name owner, asset quantity );
            ACTION transfer(     name from, name to, asset quantity, string memo );
            ACTION transferfree( name from, name to, asset quantity, string memo );
            ACTION open(         name owner, const symbol& symbol, name ram_payer );
            ACTION close(        name owner, const symbol& symbol );

            inline asset get_supply(  symbol_code sym )const;
            inline asset get_balance( name owner, symbol_code sym )const;

            struct transfer_args {
                name   from;
                name   to;
                asset  quantity;
                string memo;
            };
            
        private:
            TABLE account {
                asset balance;

                uint64_t primary_key()const { return balance.symbol.code().raw(); }
            };

            TABLE currency_stats {
                asset        supply;
                asset        max_supply;
                name issuer;

                uint64_t primary_key()const { return supply.symbol.code().raw(); }
            };

            typedef eosio::multi_index<name("accounts"), account> accounts;
            typedef eosio::multi_index<name("stat"), currency_stats> stats;

            void do_issue( name to, asset quantity, string memo, bool pay_ram = true );
            void do_transfer( name from, name to, asset quantity, string memo, bool pay_ram = true );

            void sub_balance( name owner, asset value );
            void add_balance( name owner, asset value, name ram_papyer, bool pay_ram = true );
    };

    eosio::asset token::get_supply( symbol_code sym )const
    {
        stats statstable( _self, sym.raw() );
        const auto& st = statstable.get( sym.raw() );
        return st.supply;
    }

    eosio::asset token::get_balance( name owner, symbol_code sym )const
    {
        accounts accountstable( _self, owner.value );
        const auto& ac = accountstable.get( sym.raw() );
        return ac.balance;
    }

} // namespace genereos