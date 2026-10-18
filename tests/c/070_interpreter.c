// A tiny stack-machine bytecode interpreter: a dense switch (jump table)
// in a loop, a small stack array and a byte-coded program.
// expect: 0
enum op { PUSH, ADD, SUB, MUL, DUP, SWAP, JNZ, DEC, HALT, OVER, DROP };

static const unsigned char program[] = {
  /* compute 7! with a loop:  acc = 1; n = 7; do { acc *= n; } while (--n) */
  PUSH, 1, PUSH, 7,		/* acc n */
  /* loop: */
  DUP,				/* acc n n */
  SWAP,				/* acc n n  (no-op shuffle to exercise SWAP) */
  DROP,				/* acc n */
  SWAP, OVER, MUL, SWAP,	/* acc*n n */
  DEC, DUP, JNZ, 4,		/* acc n-1 ; jump to loop if n-1 != 0 */
  DROP, HALT
};

__attribute__ ((noipa)) int run (const unsigned char *code)
{
  int stack[16];
  int sp = 0;
  unsigned pc = 0;
  for (;;)
    {
      int a, b;
      switch (code[pc++])
	{
	case PUSH: stack[sp++] = code[pc++]; break;
	case ADD: b = stack[--sp]; a = stack[--sp]; stack[sp++] = a + b; break;
	case SUB: b = stack[--sp]; a = stack[--sp]; stack[sp++] = a - b; break;
	case MUL: b = stack[--sp]; a = stack[--sp]; stack[sp++] = a * b; break;
	case DUP: a = stack[sp - 1]; stack[sp++] = a; break;
	case SWAP: a = stack[sp - 1]; stack[sp - 1] = stack[sp - 2];
	  stack[sp - 2] = a; break;
	case OVER: a = stack[sp - 2]; stack[sp++] = a; break;
	case DROP: sp--; break;
	case DEC: stack[sp - 1]--; break;
	case JNZ: a = stack[--sp]; if (a) pc = code[pc]; else pc++; break;
	case HALT: return stack[sp - 1];
	default: return -1;
	}
    }
}

int main (void)
{
  return run (program) - 5040;
}
