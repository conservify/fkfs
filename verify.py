import re

file = open("log.txt", "r")

rows = [re.findall(r'allocated\s+f#(\d+)\.(\d+).(\d+)\s+(\d+)\[(\d+)\s*->\s*(\d+)\]', line) for line in open('log.txt')]
matches = [x[0] for x in rows if x != []]

class Allocation:
    def __init__(self, file, priority, version, start, end):
        self.file = file
        self.priority = priority
        self.version = version
        self.start = start
        self.end = end
        self.r = range(start, end)

    def overlaps(self, other):
        return set(self.r).intersection(other.r)

    def __repr__(self):
        return "[{:d} {:d} {:d} {:d} {:d}]".format(self.file, self.priority, self.version, self.start, self.end)

class Block:
    def __init__(self, number):
        self.number = number
        self.allocated = []

    def allocate(self, allocation):
        for existing in self.allocated:
            if existing.overlaps(allocation):
                if existing.priority < allocation.priority:
                    print [self.number, existing, allocation]
        self.allocated.append(allocation)

class Memory:
    BlockSize = 512

    def __init__(self):
        self.blocks = {}

    def allocate(self, blockNumber, allocation):
        block = self.blocks.setdefault(blockNumber, Block(blockNumber))
        block.allocate(allocation)

card = Memory()
for match in matches:
    card.allocate(int(match[3]), Allocation(int(match[0]), int(match[1]), int(match[2]), int(match[4]), int(match[5])))
