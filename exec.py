# Executes arithmetic expressions
# Supports plus '+', minus '-', multiplication '*', and division '/'
import sys
import re

class Scanner:
    def __init__(self, f):
        self.pushback_stack = []
        self.file_pos = 0
        self.token_stack = []
        self.input = f
        
    # Get the current file position
    def get_file_pos(self):
        return self.file_pos + len(self.pushback_stack)
    
    # Get the next character
    def next(self):
        if len(self.pushback_stack) > 0:
            c = self.pushback_stack.pop()
        else:
            c = self.input.read(1)
            self.file_pos += 1
        return c
    
    # Push a character into to the character pushback
    def pushback(self, c):
        self.pushback_stack.append(c)
    
    # Get the next token
    def next_t(self):
        if len(self.token_stack) > 0:
            t = self.token_stack.pop()
            return t
        else:
            c = self.next()
            type = 'unknown'
            raw = ''
            ctype = self.get_ctype(c)
            type = ctype
            if ctype == 'alpha':
                raw = self.get_alpha(c)
            elif ctype == 'num':
                raw = self.get_num(c)
            elif ctype == 'space':
                raw = self.get_space(c)
            elif ctype == 'unkown':
                raw = self.get_unknown(c)
            else:
                raw = c
            t = {'obj': 'token', 'type': type, 'raw': raw}
            return t
    
    # Push a token onto the token pushback
    def push_token(self, t):
        self.token_stack.append(t)
    
    # Which is the character's type
    def get_ctype(self, c):
        if len(c) == 0:
            return 'eof'
        elif self.is_alpha(c):
            return 'alpha'
        elif self.is_num(c):
            return 'num'
        elif self.is_space(c):
            return 'space'
        else:
            return 'unknown'
    
    # Is the character an alpha?
    def is_alpha(self, c):
        m = re.search('[a-zA-Z]', c)
        if m: return True
        else: False
    
    # Get a string of consecutive alpha characters
    def get_alpha(self, c):
        s = c
        while True:
            c = self.next()
            if self.is_alpha(c):
                s += c
            else:
                self.pushback(c)
                return s
    
    # Is the character a digit?
    def is_num(self, c):
        m = re.search('[0-9]', c)
        if m: return True
        else: return False
    
    # Get a string of consecutive digits
    def get_num(self, c):
        s = c
        while True:
            c = self.next()
            if self.is_num(c):
                s += c
            else:
                self.pushback(c)
                return s
    
    # Is the character a space?
    def is_space(self, c):
        if re.search('[\s\t\r\n]', c):
            return True
        else:
            return False
    
    # Get a string of consecutive whitespace characters
    def get_space(self, c):
        s = c
        while True:
            c = self.next()
            if self.is_space(c):
                s += c
            else:
                self.pushback(c)
                return s
    
    # Get a string of consecutive unknown characters
    def get_unknown(self, c):
        s = c
        while True:
            c = self.next()
            ctype = self.get_ctype(c)
            if ctype == 'unknown':
                s += c
            else:
                self.pushback(c)
                return s
    
    # Print all the tokens in the input program
    # This tests the scanner
    def print_tokens(self):
        i = 0
        while True:
            t = self.next_t()
            if t['type'] == 'eof': break
            i += 1
            if i >= 200:
                print "Program is too long (>= %s)" % i
                break

    # Return the next token, skipping any space along the way
    def skip_space(self):
        while True:
            t = self.next_t()
            if t['type'] != 'space': return t

# Parse and executor
class Parser:
    def __init__(self, scanner):
        self.scanner = scanner
    
    # Parse and execute a multiply/divide expression
    def parse_factor(self, t):
        left = t
        value = int(left['raw'])
        while True:
            op = self.scanner.skip_space()
            if not op['raw'] in ['*', '/']:
                self.scanner.push_token(op)
                break
            right = self.scanner.skip_space()
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
    
    # Parse and execute an add/subtract expression
    def parse_binom(self, t):
        value = 0
        r = self.parse_factor(t)
        if r['obj'] == 'error':
            return r
        # must be value
        value = int(r['value'])
        while True:
            op = self.scanner.skip_space()
            if not op['raw'] in ['+', '-']:
                self.scanner.push_token(op)
                break
            right = self.parse_factor(self.scanner.skip_space())
            if right['obj'] == 'value':
                if op['raw'] == '-':
                    value -= int(right['value'])
                else:
                    value += int(right['value'])
            else:
                return right
        return {'obj': 'value', 'type': 'num', 'value': value}

# Open the input stream. Either a file or stdin.
if len(sys.argv) < 2:
    print "Reading from stdin."
    f = sys.stdin
else:
    try:
        f = open(sys.argv[1], 'r')
    except:
        sys.stderr.write("Failed to open file '%s'\n" % sys.argv[1])
        sys.exit(2)

scanner = Scanner(f)
parser = Parser(scanner)

# Execute the program
def execute():
    global scanner
    global parser
    while True:
        t = scanner.skip_space()
        if t['type'] == 'eof': break
        if t['type'] == 'num':
            result = parser.parse_binom(t)
            if result['obj'] == 'value':
                print "Calculated value: %s" % result['value']
            else:
                sys.stderr.write("Unexpected result: %s\n" % result)
        else:
            sys.stderr.write("Error: expected numeric expression. Got %s.\n" % t)
            sys.exit(2)

execute()

