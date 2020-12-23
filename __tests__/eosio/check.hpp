namespace eosio {
    /**
     *  Assert if the predicate fails and use the supplied message.
     *
     *  @ingroup system
     *
     *  Example:
     *  @code
     *  eosio::check(a == b, "a does not equal b");
     *  @endcode
     */
    inline void check( bool pred, const char* msg ) {
        REQUIRE( pred );
    }
}