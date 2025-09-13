// SimpleLang -> assembly
// TEMP at 0x00, variables from 0x10 upward

// decl a -> 0x10
ldi A 0
mov M A 0x10
// decl b -> 0x11
ldi A 0
mov M A 0x11
// decl c -> 0x12
ldi A 0
mov M A 0x12
ldi A 10
mov M A 0x10
// a := [stored at 0x10]
ldi A 20
mov M A 0x11
// b := [stored at 0x11]
mov A M 0x10
mov M A 0x00
mov A M 0x11
mov B A
mov A M 0x00
add
mov M A 0x12
// c := [stored at 0x12]
mov A M 0x12
mov M A 0x00
ldi A 30
mov B A
mov A M 0x00
cmp
jz L_then_0
jmp L_end_1
L_then_0:
mov A M 0x12
mov M A 0x00
ldi A 1
mov B A
mov A M 0x00
add
mov M A 0x12
// c := [stored at 0x12]
L_end_1:
hlt
