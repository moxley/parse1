# Executes arithmetic expressions
# Supports plus '+', minus '-', multiplication '*', and division '/'
import sys
import re

pushback_stack = []
file_pos = 0
token_stack = []

if len(sys.argv) < 2:
    print "Reading from stdin."
    f = sys.stdin
else:
    try:
        f = open(sys.argv[1], 'r')
    except:
        sys.stderr.write("Failed to open file '%s'\n" % sys.argv[1])
        sys.exit(2)

def get_file_pos():
    global file_pos
    global pushback_stack
    return file_pos + len(pushback_stack)

def next():
    global pushback_stack
    global file_pos
    if len(pushback_stack) > 0:
        c = pushback_stack.pop()
    else:
        global f
        c = f.read(1)
        file_pos += 1
    return c

def pushback(c):
    global pushback_stack
    pushback_stack.append(c)

def next_t():
    global token_stack
    if len(token_stack) > 0:
        t = token_stack.pop()
        return t
    else:
        c = next()
        type = 'unknown'
        raw = ''
        ctype = get_ctype(c)
        type = ctype
        if ctype == 'alpha':
            raw = get_alpha(c)
        elif ctype == 'num':
            raw = get_num(c)
        elif ctype == 'space':
            raw = get_space(c)
        elif ctype == 'unkown':
            raw = get_unknown(c)
        else:
            raw = c
        t = {'obj': 'token', 'type': type, 'raw': raw}
        return t

def push_token(t):
    global token_stack
    token_stack.append(t)

def get_ctype(c):
    if len(c) == 0:
        return 'eof'
    elif is_alpha(c):
        return 'alpha'
    elif is_num(c):
        return 'num'
    elif is_space(c):
        return 'space'
    else:
        return 'unknown'

def is_alpha(c):
    m = re.search('[a-zA-Z]', c)
    if m: return True
    else: False

def get_alpha(c):
    s = c
    while True:
        c = next()
        if is_alpha(c):
            s += c
        else:
            pushback(c)
            return s

def is_num(c):
    m = re.search('[0-9]', c)
    if m: return True
    else: return False

def get_num(c):
    s = c
    while True:
        c = next()
        if is_num(c):
            s += c
        else:
            pushback(c)
            return s

def is_space(c):
    if re.search('[\s\t\r\n]', c):
        return True
    else:
        return False

def get_space(c):
    s = c
    while True:
        c = next()
        if is_space(c):
            s += c
        else:
            pushback(c)
            return s

def get_unknown(c):
    s = c
    while True:
        c = next()
        ctype = get_ctype(c)
        if ctype == 'unknown':
            s += c
        else:
            pushback(c)
            return s

def print_tokens():
    i = 0
    while True:
        t = next_t()
        if t['type'] == 'eof': break
        i += 1
        if i >= 200:
            print "Program is too long (>= %s)" % i
            break

def skip_space():
    while True:
        t = next_t()
        if t['type'] != 'space': return t

def parse_factor(t):
    left = t
    value = int(left['raw'])
    while True:
        op = skip_space()
        if not op['raw'] in ['*', '/']:
            push_token(op)
            break
        right = skip_space()
        if right['type'] == 'num':
            right_val = int(right['raw'])
            if op['raw'] == '/':
                if right_val == 0: return {'obj': 'error', 'message': 'Divide by zero'}
                value /= right_val
            else:
                value *= right_val
        else:
            return {'obj': 'error', 'message': "Factor Expression: Expected number, but got: %s" % op}
    return {'obj': 'value', 'type': 'num', 'value': value}

def parse_binom(t):
    value = 0
    r = parse_factor(t)
    if r['obj'] == 'error':
        return r
    # must be value
    value = int(r['value'])
    while True:
        op = skip_space()
        if not op['raw'] in ['+', '-']:
            push_token(op)
            break
        right = parse_factor(skip_space())
        if right['obj'] == 'value':
            if op['raw'] == '-':
                value -= int(right['value'])
            else:
                value += int(right['value'])
        else:
            return right
    return {'obj': 'value', 'type': 'num', 'value': value}

def execute():
    while True:
        t = skip_space()
        if t['type'] == 'eof': break
        if t['type'] == 'num':
            result = parse_binom(t)
            if result['obj'] == 'value':
                print "Calculated value: %s" % result['value']
            else:
                print "Unexpected result: %s" % result
        else:
            print "Error: expected numeric expression. Got %s" % t
            break;

execute()

