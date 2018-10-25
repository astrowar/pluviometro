def get_bit(x, n):
    return (x >> n) & 0x01  # copy from arduino


def get_bits(s):
    for c in s:
        for i in range(0, 8):
            yield get_bit(ord(c), i)


def to_bit_stream(s):
    return list(get_bits(s))


def decode_bits(b8):
    x = sum([(b8[i] << i) for i in range(8)])
    return chr(x)
    # n = int('0b110100001100101011011000110110001101111', 2)
    # n.to_bytes((n.bit_length() + 7) // 8, 'big').decode()


def decode_bits_offset(b8, dx):
    x = 0
    n = len(b8)
    j = dx
    for i in range(8):
        if j >= n:
            j = 0
        x += b8[j] << i
        j += 1
    return chr(x)


def decode_stream_offset(b8, dx):
    nchars = int(len(b8) / 8)
    s = [decode_bits_offset(b8, k * 8 + dx) for k in range(nchars)]
    return ''.join(s)


def decode_stream_align(b8):
    for dx in range(8):
        sd = decode_stream_offset(b8, dx)
        idx = sd.find('\\')
        if (idx >= 0):
            return sd[idx:]
    return decode_stream_offset(b8, 0)



def is_same_zz(seq1 , seq2):
    print('==', seq1, seq2)
    if seq1 == seq2:
        return True
    return False


def is_same_z(x,n):
    print(n,x)
    if (4*n >len(x) ):
        return False
    if (is_same_zz(x[:n], x[n:2*n] )):
        if (is_same_zz(x[2*n:2*n+n], x[3*n:3*n+n])):
            return True
    return False

def get_base_sequence(x):
    nmax  = int(len(x)/2)
    print(nmax)
    for  nn in range(nmax,1,-1):
        if is_same_z(x,nn):
           return x[:nn]
    return []

def get_events(data_n ):
    for dx in range(9):
        s = decode_stream_offset(data_n, dx)
        if '20' in s:
            if ':' in s:
                print(s)
                idx = s.find(' ') + 1
                ss = s[idx:].split(' ')
                print(ss)
                # encontra o ponto de repeticao
                seq = get_base_sequence(ss)
                return seq



data = [1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0]

print(decode_bits_offset(data, 2))
print(decode_stream_offset(data, 8))

data_n = to_bit_stream("2018:05:28:8:40:12 2018:05:28:8:47:25 2018:05:28:9:40:11 2018:05:28:11:40:53 ")

data_n = data_n + data_n + data_n + data_n + data_n
data_n = data_n[2:]

#o data_n precisa ser maior que o conteudo dos dados, umas 5 vezes

print("------------------------------")
print(" data stream : ", ''.join([str(x) for x in data_n]))


print(decode_stream_align(data_n))
seq_base = get_events(data_n)

print(seq_base)
