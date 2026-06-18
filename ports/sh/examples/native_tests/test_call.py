def helper(x):
    return x * 2

@micropython.native
def call_test(a):
    return helper(a)

print("call_test(5) =", call_test(5))
