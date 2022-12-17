import time
t1 = time.time()

for j in range(500):
  f = 1
  for i in range(2, 250):
    f *= i
print(f)

t2 = time.time()
print(f"{t2-t1} seconds")
