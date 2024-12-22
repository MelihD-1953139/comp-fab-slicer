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
    plt.gca().set_aspect("equal", adjustable="box")
    plt.show()


def plot_points(points, color="blue"):
    x_values = [point[0] for point in points]
    y_values = [point[1] for point in points]
    plt.plot(x_values, y_values, marker="o", color=color)


colors = ["blue", "green", "red", "cyan", "m", "y", "k", "w"]

if __name__ == "__main__":
    input_points = """(157.299, 182.926) (157.701, 182.524) (157.701, 159.074) (133.847, 159.074) (133.847, 182.927) (157.299, 182.926)
(101.153, 182.927) (101.153, 159.074) (77.2997, 159.074) (77.3009, 182.524) (77.703, 182.926) (101.153, 182.927)
(157.701, 102.526) (133.847, 102.526) (133.847, 126.38) (157.701, 126.38) (157.701, 102.526)
(121.643, 43.403) (121.356, 46.4294) (121.33, 46.704) (120.793, 50.6974) (120.716, 51.266) (120.668, 51.5385) (120.368, 53.2309) (120.304, 53.5338) (120, 54.9657) (119.921, 55.2665) (119.612, 56.4713) (119.52, 56.7557) (119.21, 57.6961) (119.107, 57.9351) (118.794, 58.6743) (118.682, 58.8582) (118.368, 59.3766) (118.249, 59.491) (117.936, 59.789) (117.814, 59.8307) (117.5, 59.9363) (117.186, 59.8307) (117.064, 59.789) (116.751, 59.491) (116.632, 59.3766) (116.318, 58.8582) (116.206, 58.6743) (115.893, 57.9351) (115.79, 57.6961) (115.48, 56.7557) (115.388, 56.4713) (115.079, 55.2665) (115, 54.9657) (114.696, 53.5338) (114.632, 53.2309) (114.334, 51.5385) (114.286, 51.266) (113.708, 46.9828) (113.67, 46.704) (113.359, 43.403) (114.201, 43.403) (114.596, 43.403) (115.178, 43.403) (115.442, 43.403) (115.562, 43.403) (115.844, 43.403) (115.902, 43.403) (116.196, 43.403) (116.211, 43.403) (116.352, 43.403) (116.474, 43.7844) (116.724, 44.3571)"""

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

    input_lines = """Line: 112.2, 142.5 -> 142.5, 142.5
Line: 112.2, 142.5 -> 92.5, 142.5
Line: 142.5, 122.8 -> 142.5, 92.5
Line: 142.5, 122.8 -> 142.5, 142.5
Line: 122.8, 92.5 -> 92.5, 92.5
Line: 122.8, 92.5 -> 142.5, 92.5
Line: 92.5, 112.2 -> 92.5, 142.5
Line: 92.5, 112.2 -> 92.5, 92.5"""

    lines = parse_lines(input_lines)
    plot_lines(lines)
