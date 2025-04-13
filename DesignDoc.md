1. No global variables, only if there is no other way
2. Every Data(or State that our app needs) is allocated in main() on stack
3. Functions will operate on this Data
4. Anything that isn't mutable should be const
5. It is the responsibility of the function to print error messages, not the caller (the caller should exit)
6. If we have nested functions, the inner function is responsible for printing error messages for its own work
7. Every error should be written to stderr
8. You can control whether a function will print errors with the verbose argument (some functions)
9. Linux kernel coding style
