program        ::= { statement }

statement      ::= declaration | assignment | conditional

declaration    ::= "int" identifier ";"

assignment     ::= identifier "=" expression ";"

conditional    ::= "if" "(" condition ")" "{" { statement } "}"

expression     ::= term { ("+" | "-") term }

term           ::= identifier | number

condition      ::= identifier "==" identifier

identifier     ::= letter { letter | digit }
number         ::= digit { digit }

letter         ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
digit          ::= "0" | "1" | ... | "9"
