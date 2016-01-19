#!/usr/bin/python
f = open('preflop.txt')
output = open('preflop.data', 'w')

lines = f.readlines()
f.close()

header = """#ifndef __PREFLOP_HPP__
#define __PREFLOP_HPP__

#include <map>
#include <string>

map<string, int> preflop = {%s};
"""

footer = """#endif  // __PREFLOP_HPP__
"""

res = {}
entry_base = '{"%s", %d}'

def full_hand(cur_hand):
    if '-' not in cur_hand and 'x' not in cur_hand and 'y' not in cur_hand:
        cards = sorted(cur_hand.split(' '))
        res[''.join(cards)] = int(float(splits[3])*10000)

suits = ['s', 'h', 'c', 'd']

for line in lines[1:]:
    splits = line.split('\t')
    if len(splits) > 12:
        continue
    hand = splits[1]

    for suit1 in suits:
        new_hand1 = hand
        avail_suits = ['s', 'h', 'c', 'd']
        if 'x' in hand:
            avail_suits.remove(suit1)
            new_hand1 = new_hand1.replace('x', suit1)
        for suit2 in [x for x in suits if x != suit1]:
            new_hand2 = new_hand1
            if 'y' in new_hand2:
                new_hand2 = new_hand2.replace('y', suit2)
                avail_suits.remove(suit2)
            full_hand(new_hand2)

            for misc1 in avail_suits:
                cur_hand1 = new_hand2
                cur_hand1 = cur_hand1.replace('-', misc1, 1)
                full_hand(cur_hand1)
                for misc2 in [x for x in avail_suits if x != misc1]:
                    cur_hand2 = cur_hand1.replace('-', misc2, 1)
                    full_hand(cur_hand2)
                    for misc3 in [x for x in avail_suits if x != misc1 and x != misc2]:
                        cur_hand3 = cur_hand2.replace('-', misc3, 1)
                        full_hand(cur_hand3)
                        for misc4 in [x for x in avail_suits if x != misc1 and x != misc2 and x != misc3]:
                            cur_hand4 = cur_hand3.replace('-', misc4, 1)
                            full_hand(cur_hand4)

for k in res:
    output.write(k)
    output.write(' ')
    output.write(str(res[k]) + '\n')
output.close()
