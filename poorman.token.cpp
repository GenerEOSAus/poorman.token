/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "include/poorman.token.hpp"

namespace genereos {

    void token::create( name issuer, asset maximum_supply ) 
    {
        require_auth( _self );

        auto sym = maximum_supply.symbol;
        check( sym.is_valid(),            "Invalid symbol name"             );  
        check( maximum_supply.is_valid(), "Invalid supply"                  ); 
        check( maximum_supply.amount > 0, "Maximum supply must be positive" );    
        
        auto sym_code_raw = sym.code().raw();

        stats statstable( _self, sym_code_raw );
        auto existing = statstable.find( sym_code_raw );
        check( existing == statstable.end(), "Token with that symbol name exists" );

        statstable.emplace( _self, [&]( auto& s ) 
        {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply    = maximum_supply;
            s.issuer        = issuer;
        });
    }

    void token::issue( name to, asset quantity, string memo )
    {
        do_issue( to, quantity, memo, true );
    }

    void token::issuefree( name to, asset quantity, string memo )
    {
        do_issue( to, quantity, memo, false );
    }

    void token::burn( name from, asset quantity, string memo )
    {
        auto sym = quantity.symbol;
        check( sym.is_valid(),     "Invalid symbol name"                   );
        check( memo.size() <= 256, "Memo must be less than 256 characters" );

        auto sym_code_raw = sym.code().raw();

        stats statstable( _self, sym_code_raw );
        auto existing = statstable.find( sym_code_raw );
        check( existing != statstable.end(), "Token with that symbol name does not exist - Please create the token before burning" );

        const auto& st = *existing;

        require_auth( from );
        require_recipient( from );
        check( quantity.is_valid(), "Invalid quantity value" );
        check( quantity.amount > 0, "Quantity value must be positive" );

        check( st.supply.symbol == quantity.symbol, "Symbol precision mismatch" );
        check( st.supply.amount >= quantity.amount, "Quantity value cannot exceed the available supply" );

        statstable.modify( st, same_payer, [&]( auto& s ) 
        {
            s.supply -= quantity;
        });

        sub_balance( from, quantity );
    }

    void token::signup( name owner, asset quantity )
    {
        auto sym = quantity.symbol;
        check( sym.is_valid(), "Invalid symbol name" );

        auto sym_code_raw = sym.code().raw();

        stats statstable( _self, sym_code_raw );
        auto existing = statstable.find( sym_code_raw );
        check( existing != statstable.end(), "Token with that symbol name does not exist - Please create the token before issuing" );

        const auto& st = *existing;

        require_auth( owner );
        require_recipient( owner );

        accounts to_acnts( _self, owner.value );
        auto to = to_acnts.find( sym_code_raw );
        check( to == to_acnts.end(), "You have already signed up" );

        check( quantity.is_valid(), "Invalid quantity value" );
        check( quantity.amount == 0, "Quantity exceeds signup allowance" );
        check( st.supply.symbol == quantity.symbol, "Symbol precision mismatch" );
        check( st.max_supply.amount - st.supply.amount >= quantity.amount, "Quantity value cannot exceed the available supply" );

        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply += quantity;
        });

        add_balance( owner, quantity, owner );
    }

    void token::transfer( name from, name to, asset quantity, string memo )
    {
    do_transfer( from, to, quantity, memo, true );
    }

    void token::transferfree( name from, name to, asset quantity, string memo )
    {
    do_transfer( from, to, quantity, memo, false );
    }

    void token::do_issue( name to, asset quantity, string memo, bool pay_ram )
    {
        auto sym = quantity.symbol;
        check( sym.is_valid(),     "Invalid symbol name"                   );
        check( memo.size() <= 256, "Memo must be less than 256 characters" );
 
        auto sym_code_raw = sym.code().raw();

        stats statstable( _self, sym_code_raw );
        auto existing = statstable.find( sym_code_raw );
        check( existing != statstable.end(), "Token with that symbol name does not exist - Please create the token before issuing" );

        const auto& st = *existing;

        require_auth( st.issuer );
        check( quantity.is_valid(), "Invalid quantity value" );
        check( quantity.amount > 0, "Quantity value must be positive" );

        check( st.supply.symbol == quantity.symbol, "Symbol precision mismatch" );
        check( st.max_supply.amount - st.supply.amount >= quantity.amount, "Quantity value cannot exceed the available supply" );

        statstable.modify( st, same_payer, [&]( auto& s ) 
        {
            s.supply += quantity;
        });

        add_balance( st.issuer, quantity, st.issuer );

        if (to != st.issuer) {
            if (pay_ram == true) {
                SEND_INLINE_ACTION( *this, transfer, { st.issuer, name("active") }, { st.issuer, to, quantity, memo } );
            } else {
                SEND_INLINE_ACTION( *this, transferfree, { st.issuer, name("active") }, { st.issuer, to, quantity, memo } );
            }
        }
    }

    void token::do_transfer( name from, name to, asset quantity, string memo, bool pay_ram )
    {
        require_auth( from );

        check( from != to,       "Cannot transfer to self"   );
        check( is_account( to ), "to Account does not exist" );

        auto sym = quantity.symbol;

        stats statstable( _self, sym.code().raw() );
        const auto& st = statstable.get( sym.code().raw(), "Token with that symbol name does not exist - Please create the token before transferring" );        

        require_recipient( from );
        require_recipient( to );

        check( quantity.is_valid(), "Invalid quantity value"          );
        check( quantity.amount > 0, "Quantity value must be positive" );

        check( st.supply.symbol == quantity.symbol, "Symbol precision mismatch"             );
        check( memo.size() <= 256,                  "Memo must be less than 256 characters" );

        sub_balance( from, quantity );
        add_balance( to, quantity, from, pay_ram );
    }

    void token::open( name owner, const symbol& symbol, name ram_payer )
    {
        require_auth( ram_payer );

        auto sym_code_raw = symbol.code().raw();

        stats statstable( _self, sym_code_raw );
        const auto& st = statstable.get( sym_code_raw, "Symbol does not exist" );
        check( st.supply.symbol == symbol, "Symbol precision mismatch" );

        accounts acnts( _self, owner.value );
        auto it = acnts.find( sym_code_raw );

        if ( it == acnts.end() ) {
            acnts.emplace( ram_payer, [&]( auto& a ) {
                a.balance = asset{0, symbol};
            });
        } 
    }

    void token::close( name owner, const symbol& symbol )
    {
        require_auth( owner );
        accounts acnts( _self, owner.value );

        auto it = acnts.find( symbol.code().raw() );
        check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect" );
        check( it->balance.amount == 0, "Cannot close because the balance is not zero" );

        acnts.erase( it );
    }

    void token::sub_balance( name owner, asset value ) 
    {
        accounts from_acnts( _self, owner.value );
        auto sym  = value.symbol;
        auto from = from_acnts.find( sym.code().raw() );
        check( from != from_acnts.end(), "No balance object found under the token balance owner's account" );

        const auto& from_iter = *from;
        check( from_iter.balance.amount >= value.amount, "Overdrawn balance will result in a nonexistent negative balance of the account" );

        if ( from_iter.balance.amount == value.amount ) {
            from_acnts.erase( from_iter );
        } else {
            from_acnts.modify( from_iter, owner, [&]( auto& a ) 
            {
                a.balance -= value;
            });
        }
    }

    void token::add_balance( name owner, asset value, name ram_payer, bool pay_ram )
    {
        accounts to_acnts( _self, owner.value );        
        auto to = to_acnts.find( value.symbol.code().raw() );

        if ( to == to_acnts.end() ) {
            check( pay_ram == true, "Destination account does not have balance" );
            to_acnts.emplace( ram_payer, [&]( auto& a ) 
            {
                a.balance = value;
            });
        } else {
            to_acnts.modify( to, same_payer, [&]( auto& a ) 
            {
                a.balance += value;
            });
        } 
    }

} // namespace genereos

EOSIO_DISPATCH( genereos::token, (create)(issue)(issuefree)(burn)(signup)(transfer)(transferfree)(open)(close) )