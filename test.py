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


def plot_points(points, color="blue"):
    x_values = [point[0] for point in points]
    y_values = [point[1] for point in points]
    plt.plot(x_values, y_values, marker="o", color=color)


colors = ["blue", "green", "red", "cyan", "m", "y", "k", "w"]

if __name__ == "__main__":
    input_points = """(141.9, 93.1) (141.9, 141.9) (93.1, 141.9) (93.1, 93.1) (141.9, 93.1)
(141.9, 93.1) (141.9, 141.9) (93.1, 141.9) (93.1, 93.1) (141.9, 93.1)
(141.9, 93.1) (141.9, 141.9) (93.1, 141.9) (93.1, 93.1) (141.9, 93.1)
(141.9, 93.1) (141.9, 141.9) (93.1, 141.9) (93.1, 118.1) (118.1, 118.1) (118.1, 93.1) (141.9, 93.1)
(141.898, 141.898) (93.1016, 141.898) (93.1016, 118.102) (118.102, 118.102) (118.102, 93.1016) (141.898, 93.1016)
(118.102, 93.1016) (118.102, 118.102) (93.1016, 118.102) (93.1016, 93.1016)"""

    lines = input_points.strip().split("\n")
    num_lines = len(lines)

    # Calculate the min and max values for x and y coordinates
    all_points = []
    for line in lines:
        points = parse_points(line)
        all_points.extend(points)

    x_values = [point[0] for point in all_points]
    y_values = [point[1] for point in all_points]
    x_min, x_max = min(x_values) - 5, max(x_values) + 5
    y_min, y_max = min(y_values) - 5, max(y_values) + 5

    # Create a figure for individual plots
    plt.figure(figsize=(10, 2 * num_lines))

    for i, line in enumerate(lines):
        points = parse_points(line)
        plt.subplot(1, num_lines + 1, i + 1)
        plot_points(points, color="red")
        plt.xlabel("X-axis")
        plt.ylabel("Y-axis")
        plt.title(f"Line {i + 1}")
        plt.grid(True)
        plt.gca().set_aspect("equal", adjustable="box")
        plt.xlim(x_min, x_max)
        plt.ylim(y_min, y_max)

    # Create a combined plot
    plt.subplot(1, num_lines + 1, num_lines + 1)
    for line in lines:
        points = parse_points(line)
        plot_points(points, color="blue")
    plt.xlabel("X-axis")
    plt.ylabel("Y-axis")
    plt.title("Combined Plot")
    plt.grid(True)
    plt.gca().set_aspect("equal", adjustable="box")
    plt.xlim(x_min, x_max)
    plt.ylim(y_min, y_max)

    plt.tight_layout()
    plt.show()
