from sympy.ntheory.factor_ import totient 
import sympy
import random

psize = 10000
n = int(input("n primes: "))
m = int(input("m primes: "))

# Trent work
pn = []
pm = []

delta = 1
alpha = 1
beta = 1
temp_q = 1

for i in range(m):
    p = sympy.sieve[random.randrange(psize)]
    pm.append(p)
    temp_q *= p

for i in range(n):
    p = sympy.sieve[random.randrange(psize)]
    while p in pm:
        p = sympy.sieve[random.randrange(psize)]
    pn.append(p)
    delta *= pow(p, 3)
    alpha *= pow(p, 2)*(p-1)
    beta *= p*(p-1)*temp_q

assert pow(beta,2) % alpha == 0, 'alpha does not divide beta^2'
assert beta % alpha != 0, 'alpha divides beta'

print('beta % alpha', beta % alpha)

print("pn: {0}\npm: {1}\n".format(pn, pm))

totient_delta = totient(delta)
y = random.randrange(delta)

while sympy.igcd(y, delta) != 1:
    y = random.randrange(delta)

print("delta: {0}\n beta: {1}\n alpha: {2}\n y: {3}".format(delta, beta, alpha, y))

# Alice work (recebe delta, beta e alpha de trent)
xa = random.randrange(1,delta)
xb = random.randrange(1,totient_delta)
while xb*beta % totient_delta == 0:
    xb = random.randrange(totient_delta)
gamma = alpha*pow(xa,2) + beta*xb

# Bob work (recebe gamma e delta de alice)
xa1 = random.randrange(1,delta)
xb1 = random.randrange(1,totient_delta)
while xb1*gamma % totient_delta == 0:
    xb1 = random.randrange(totient_delta)
gamma1 = alpha*pow(xa1,2) + beta*xb1

# Calcula chave de sessao
kab = pow(y, gamma1 * xb, delta)
kba = pow(y, gamma * xb1, delta)

assert kab == kba, 'Chaves de sessao sao diferentes'
print("\nkab: {0}\nkba: {1}".format(kab, kba))

# Carlos entra na rede
# alice calcula gamma2 e xa2
xa2 = random.randrange(1,delta)
gamma2 = alpha*pow(xa2, 2) + beta*kab

# Alice escolhe y
y = random.randrange(delta)
while sympy.igcd(y, delta) != 1:
    y = random.randrange(delta)

# Carlos escolhe xc1, xc2 e calcula gamma3
xc1 = random.randrange(1,delta)
xc2 = random.randrange(1,totient_delta)
while xc2*gamma2 % totient_delta == 0:
    xc2 = random.randrange(totient_delta)
gamma3 = alpha*pow(xa2, 2) + beta*xc2

# alice e bob calculam kabc
kabc = pow(y, gamma3*kab, delta)

#carlos calcula kabc
kabc1 = pow(y, gamma2*xc2, delta)

print("\nkabc: {0}\nkabc1: {1}".format(kabc, kabc1))
