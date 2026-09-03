# infrastructure/testing/

The one assertion helper every module's tests use, so a failed check prints
what was expected and what arrived in the same shape everywhere. It holds no
tests of its own; each module keeps its tests beside its code, in its own
`tests/`.

In this folder:

- [`check.h`](check.h): `check::equal` and `check::text`, plus the counter a
  test's `main` returns.

Example of use, the shape every test file here takes:

```cpp
#include "check.h"

int main() {
    check::equal("the target is read", parsed.target, 7);
    check::text("the positional word is the file", parsed.file, "tensor.sms");
    return check::report("arguments");   // the tally, nonzero on any failure
}
```

Why it sits directly under `infrastructure/` instead of inside a module: the
top of [`CMakeLists.txt`](CMakeLists.txt).
