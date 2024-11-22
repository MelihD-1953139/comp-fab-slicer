import matplotlib.pyplot as plt

def parse_line(line):
    """Parses a line in the format 'x1 y1 -> x2 y2' and returns two points."""
    [p1, p2]  = line.split('->')	
    [x1, y1] = p1.split()
    [x2, y2] = p2.split()
    return ((float(x1), float(y1)), (float(x2), float(y2)))

def plot_lines(lines):
    """Plots a list of lines on a graph."""
    for line in lines:
        x, y = zip(*line)
        plt.plot(x, y)

if __name__ == "__main__":
    lines = []
    with open('lines.txt', 'r') as f:
        for line in f:
            lines.append(parse_line(line))

    plot_lines(lines)
    plt.show()