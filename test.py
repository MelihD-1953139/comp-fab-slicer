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
    input_points = """(115.281, 98.6484) (118.719, 99.3203) (118.727, 99.3281) (120.469, 99.8672) (123.75, 101.297) (125.273, 102.18) (128.203, 104.344) (129.438, 105.492) (131.812, 108.25) (133.672, 111.188) (135.117, 114.5) (135.133, 114.531) (136.062, 118.008) (136.344, 119.742) (136.469, 123.328) (136.477, 123.328) (136.477, 123.359) (136.102, 126.891) (135.711, 128.602) (134.57, 131.953) (133.781, 133.57) (131.906, 136.609) (129.578, 139.352) (127.242, 141.398) (127.094, 141.562) (126.969, 141.742) (126.883, 141.953) (126.844, 142.172) (126.836, 142.297) (98.1328, 142.297) (98.1172, 142.133) (98.0625, 141.906) (97.9688, 141.703) (97.8438, 141.523) (97.6797, 141.367) (96.8672, 140.711) (94.2812, 138.117) (93.6328, 137.32) (93.6016, 137.281) (93.4375, 137.133) (93.2578, 137.008) (93.0469, 136.922) (92.8281, 136.883) (92.7031, 136.867) (92.7031, 108.109) (92.8672, 108.109) (93.0938, 108.062) (93.2891, 107.961) (93.4766, 107.844) (93.6328, 107.68) (94.3906, 106.727) (96.9766, 104.18) (98.3516, 103.102) (101.406, 101.211) (103.039, 100.43) (106.391, 99.2812) (108.102, 98.8906) (111.656, 98.5234)
(110.32, 106.656) (110.102, 106.688) (107.367, 107.344) (107.289, 107.367) (107.078, 107.453) (107.031, 107.477) (105.891, 107.938) (105.742, 108.008) (103.328, 109.391) (103.148, 109.523) (101.023, 111.359) (101.008, 111.367) (100.859, 111.531) (99.1016, 113.773) (99.0078, 113.898) (98.3125, 115.078) (98.2969, 115.094) (98.2188, 115.289) (97.1797, 117.898) (97.1172, 118.109) (96.5938, 120.883) (96.5938, 120.891) (96.5703, 121.109) (96.5703, 123.891) (96.5938, 124.117) (97.1172, 126.891) (97.1719, 127.047) (97.6328, 128.383) (97.6875, 128.539) (98.9688, 131.039) (99.1016, 131.227) (100.844, 133.453) (100.953, 133.578) (101.969, 134.547) (102.102, 134.656) (104.391, 136.289) (104.539, 136.383) (105.789, 137.023) (105.938, 137.078) (108.602, 138.008) (108.781, 138.062) (110.102, 138.297) (110.117, 138.312) (110.344, 138.344) (113.133, 138.469) (113.352, 138.461) (116.172, 138.07) (116.352, 138.031) (117.688, 137.633) (117.852, 137.57) (120.383, 136.406) (120.57, 136.297) (122.891, 134.656) (123.031, 134.547) (124.047, 133.578) (124.156, 133.453) (125.898, 131.227) (125.906, 131.227) (126.031, 131.039) (127.312, 128.539) (127.367, 128.383) (127.828, 127.047) (127.883, 126.891) (128.406, 124.117) (128.406, 124.109) (128.43, 123.891) (128.43, 121.109) (128.406, 120.883) (127.883, 118.109) (127.82, 117.898) (126.773, 115.258) (126.688, 115.078) (126, 113.898) (125.898, 113.773) (124.141, 111.531) (124.141, 111.523) (123.977, 111.359) (121.852, 109.523) (121.672, 109.391) (119.258, 108.008) (119.109, 107.938) (117.867, 107.438) (117.828, 107.422) (117.617, 107.344) (114.891, 106.68) (114.742, 106.656) (114.531, 106.648) (113.32, 106.531) (113.141, 106.523)"""

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

    # input_lines = """Line: 112.2, 142.5 -> 142.5, 142.5
# Line: 112.2, 142.5 -> 92.5, 142.5
# Line: 142.5, 122.8 -> 142.5, 92.5
# Line: 142.5, 122.8 -> 142.5, 142.5
# Line: 122.8, 92.5 -> 92.5, 92.5
# Line: 122.8, 92.5 -> 142.5, 92.5
# Line: 92.5, 112.2 -> 92.5, 142.5
# Line: 92.5, 112.2 -> 92.5, 92.5"""

#     lines = parse_lines(input_lines)
#     plot_lines(lines)
