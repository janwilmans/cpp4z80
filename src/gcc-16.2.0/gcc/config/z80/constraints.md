;; Z80 register constraints

(define_register_constraint "r" "GENERAL_REGS"
  "A general-purpose Z80 register.")

(define_constraint "n"
  "A constant integer."
  (match_code "const_int"))

(define_constraint "X"
  "No constraint."
  (match_test "true"))
