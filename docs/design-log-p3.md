# Project 3 Design Log
**Author:** Brant Willever · **P2 code used:** My own
**Tools used:** I used ChatGPT to help interpret the spec and for help with understanding test results.

## 1. Hash table

To implement the hash table, I used separate chaining, with nested vectors holding the buckets. Chaining stores tools with the same bucket index together, making collisions easy to handle. I chose FNV-1a as a hash algorithm because it processes every character in order rather than using a small part of the name. I made the table start with 16 buckets, with a maximum load factor of 1. If an insertion were to exceed this value, the bucket count would double and recalculate the bucket position of each existing tool.

With 10,000 tools named "tool_0" through "tool_9999", the test measured 16,384 buckets, a load factor of 0.610, and a maximum chain length of 6. The weak hash "s[0] + s.size()" produced a maximum chain length of 9,000 given these same names and bucket count, which shows that using a real string hash is more effective.

When the hash spreads names across buckets evenly, the average cost of adding a tool remains constant. Duplicate checking only examines a single chain, meaning that adding a tool to a chain is a constant operation on average. A resize operation is proportional to the number of tools stored in the hash table, but since the table only doubles when it becomes full (load factor of 1), the insertions between these doublings cause enough constant time operations to occur that it spreads out the rehash operations enough to amortize to O(1). Essentially, each resize costs O(n), but because the table is also doubling, the number of insertions between resizes increases, so the average insertion cost becomes O(1).

## 2. Pointer stability

Each bucket stores a tool object, which is pointed to by a unique pointer. Using the "find()" operations points to the "Tool" allocated on the heap, not to an actual bucket or vector element. During a resizing operation, each unique pointer is moved into the new bucket array. However, because this does not move the actual "Tool" object, the "Tool*" pointer that was previously returned keeps the same address. The scale test saves the pointer to the first tool, inserts the remaining tools through multiple resizes, and makes sure that the same pointer is still returned using "find()".

## 3. Calculator trace

The calculator traces a values stack and an operators stack. First, the '2' is pushed onto values. Next, the '*' is pushed onto operators, which is read as multiplication. The '(' is read, and then '3' is pushed onto values. The '+' is pushed onto operators, which puts addition next due to the parenthesis. The '4' is pushed onto values, and then the ')' is read, which causes addition to be applied and the "3 + 4" operation occurs, where 3 and 4 are popped from the values stack and 7 is then pushed. Next, the left parenthesis is removed, and the values '2' and '7' are left with the operator '*'. The '-' operation is then read, but since multiplication has higher precedence, the "2 * 7" operation occurs, where 2 and 7 are popped from the values stack and 14 is pushed. After this, the '-' operator is pushed to the operators stack. Finally, '5' is read and pushed onto the values stack. The subtraction operation pops 5 and 14, with the final result being 9.

## 4. Errors as values

When an agent uses tools, errors are often common, and are often not the result of programming mistakes. Many things might occur due to agentic errors, such as missing files, division by 0, or rejected workspace paths. Tools can return error values that contain error messages, and "resolve()" and "calc_evaluate()" can return "false" because they return boolean values as status indicators. The harness can then give this error information to the model, which is able to account for these errors and adjust its work instead of terminating.

Exceptions should be used for actual programming errors or when a function's purpose is violated. Exceptions are more useful for bugs that need to be fixed within the program's code, or for edge cases that are not accounted for properly. In the project, the calculator checks its stacks before accessing their elements, so that malformed token sequences return errors instead of throwing an exception.

## 5. What I would design differently

If I were to rewrite this program now from scratch, I would probably want to define the initial bucket count and maximum load factor as actual named constants in the program, rather than as simple values. This would make it easier to maintain for resizing and would allow for easier access to these values. Another thing I could add would be a test focused on when the load factor is reached and a resize is triggered. This would allow for easier finding of errors in my logic related to these values.