@micropython.native
def loop_test(n):
    sum = 0
    for i in range(n):
        sum = sum + i
    return sum

print("sum(10) =", loop_test(10))
print("sum(100) =", loop_test(100))
