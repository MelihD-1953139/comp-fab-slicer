import matplotlib.pyplot as plt


def parse_lines(input_lines):
    lines = []
    for line in input_lines.split("\n"):
        if line.startswith("Line:"):
            # Remove "Line: " and split by " -> "
            points = line[6:].split(" -> ")
            # Extract x and y coordinates for both points
            x1, y1 = map(float, points[0].split(", "))
            x2, y2 = map(float, points[1].split(", "))
            lines.append(((x1, y1), (x2, y2)))
    return lines


# (119.92, 85.2) (134.8, 100.08) (134.8, 134.8) (85.2, 134.8) (85.2, 85.2)
def parse_points(input_points):
    points = []
    for point in input_points.split(")"):
        if point:
            point = point.lstrip(" ()\n")
            x, y = map(float, point.split(","))
            points.append((x, y))
    return points


def plot_lines(lines):
    for line in lines:
        (x1, y1), (x2, y2) = line
        plt.plot([x1, x2], [y1, y2], marker="o")

    plt.xlabel("X-axis")
    plt.ylabel("Y-axis")
    plt.title("Line Plot")
    plt.grid(True)
    plt.show()


def plot_points(points):
    x_coords = [point[0] for point in points]
    y_coords = [point[1] for point in points]

    # Add the first point at the end to close the loop
    x_coords.append(points[0][0])
    y_coords.append(points[0][1])

    plt.plot(x_coords, y_coords, marker="o")

    plt.xlabel("X-axis")
    plt.ylabel("Y-axis")
    plt.title("Point Plot")
    plt.grid(True)


if __name__ == "__main__":
    input_lines = """
Line: 119.92, 85.2 -> 134.8, 100.08
Line: 134.8, 100.08 -> 134.8, 134.8
Line: 134.8, 134.8 -> 85.2, 134.8
Line: 85.2, 134.8 -> 85.2, 85.2
Line: 85.2, 85.2 -> 119.92, 85.2
"""
    input_points = """(85.2031, 134.797) (134.797, 85.2031) 
(85.2031, 134.703) (85.2969, 134.797) 
(134.797, 92.2734) (92.2734, 134.797) 
(92.375, 134.797) (85.2031, 127.625) 
(99.3438, 134.797) (134.797, 99.3359) 
(85.2031, 120.562) (99.4375, 134.797) 
(134.797, 106.422) (106.422, 134.797) 
(106.516, 134.797) (85.2031, 113.484) 
(113.484, 134.797) (134.797, 113.484) 
(85.2031, 106.422) (113.578, 134.797) 
(134.797, 120.562) (120.562, 134.797) 
(120.656, 134.797) (85.2031, 99.3438) 
(127.625, 134.797) (134.797, 127.625) 
(85.2031, 92.2734) (127.727, 134.797) 
(134.797, 134.703) (134.703, 134.797) 
(134.797, 134.797) (85.2031, 85.2031) 
(127.727, 85.2031) (85.2031, 127.727) 
(92.2734, 85.2031) (134.797, 127.727) 
(85.2031, 120.656) (120.656, 85.2031) 
(134.797, 120.656) (99.3438, 85.2031) 
(113.578, 85.2031) (85.2031, 113.578) 
(106.422, 85.2031) (134.797, 113.578) 
(85.2031, 106.516) (106.516, 85.2031) 
(134.797, 106.516) (113.484, 85.2031) 
(99.4375, 85.2031) (85.2031, 99.4375) 
(120.562, 85.2031) (134.797, 99.4375) 
(85.2031, 92.375) (92.375, 85.2031) 
(134.797, 92.375) (127.625, 85.2031) 
(85.2969, 85.2031) (85.2031, 85.2969) 
(134.703, 85.2031) (134.797, 85.2969)"""

    for line in input_points.split("\n"):
        if line != "":
            points = parse_points(input_points)
            plot_points(points)
    plt.show()
