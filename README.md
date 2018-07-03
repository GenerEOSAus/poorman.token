# poorman.token
EOS Token Contract with "Burning" and "Signup" built in (for making the token RAM cheap)

This token uses the standard eosio.token contract with two modifications.

### To signup for an airdrop:
`cleos push action poor.token signup '{"owner":"iwanttokens1","quantity":"0.0000 POOR"}' -p iwanttokens1@active`

The signup function allows an account to create a balance entry using their own personal ram.

They signup restricts the quantity to 0 however you could configure this in the contract code to allow a "signup bonus".

You would have to add some controls to prevent double signups though, so 0 quantity signup is safest.

### To burn tokens run the command:
`cleos push action poor.token burn '{"from":"tokensowner1","quantity":"1.0000 POOR","memo":"Lets remove POOR supply!"}' -p tokensowner1@active`

The burn function burns the token from the "from account" and also reduces the supply.

The burn function makes sure you can't burn more tokens than supply.
