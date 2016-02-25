import sys
import random
import getopt

test_case = 0
def start(opt, argv):

    random.seed()
    N = int(argv[0])

    expected = 1
    if len(opt) == 0:
       is_code = False
    else:
       is_code = True
    for n_oper in range(1, N + 1):
        print '// %d operator%s ' % (n_oper, 's' if n_oper > 1 else '')
        for m in range(0, 10):
            generate_expression(n_oper, expected, list(argv[1]), is_code)
            expected = expected + 1

def generate_expression(num_oper, desired_result, opers, is_code):
    global test_case
    expr = ""
    for k in range (0, num_oper-1):
        expr += "{0}.{1} {2} ".format(random.randint (1, 200),
                                          random.randint(0,9),
                                          random.choice(opers))
    expr += "{0}.0".format(random.randint(1,20))
    left_result = eval(expr)

    # calculate the last (additive) term in order to get the desired result
    while True:
        next_op = random.choice(['+', '-'])
        if next_op == '+':
          next_val = desired_result - left_result
        else:
          next_val = left_result - desired_result
        if next_val >= 0:
            break
    expr += " {0} {1}".format(next_op, next_val)
    if is_code:
        print 'printf("Test {0:2d} ===> %7.3f\\n", evaluate("{1}", true));'.format(test_case + 1, expr)
    else:
        print '{0}'.format(expr)
    test_case = test_case + 1

def print_usage():
    print 'usage: genarith.ph --code <max-number-of-operators> <operators>'

if __name__ == "__main__":
    try:
        opts,args = getopt.getopt(sys.argv[1:], "-c", ["code"])
    except getopt.GetoptError as err:
        print str(err)
        print_usage()
        sys.exit(1)

    if len(args) < 2:
        print_usage()
        sys.exit(1)

    start(opts, args)
