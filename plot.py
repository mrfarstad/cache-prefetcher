import matplotlib.pyplot as plt

stop_words = [
    "swim",
    "ammp",
    "twolf",
    "bzip2_source",
    "bzip2_program",
    "bzip2_graphic",
    "art110",
    "art470",
    "wupwise",
    "appsi",
    "applu",
    "galgel",
]


def harmonic_mean(speedups):
    mean = 0
    for speedup in speedups:
        mean += 1 / speedup
    return len(speedups) / mean


path = "stats"
nums = [2, 4, 8, 16]

best = []
best_mean = 0

# for degree in nums:
degree = 4
depth = 2
# for depth in nums:
means = []
for width in nums:
    with open(
            path + "/degree_" + str(degree) + "/depth_" + str(depth) +
            "/width_" + str(width) + "/stats.txt", "r") as file:
        lines = file.readlines()
        for i in range(0, len(lines)):
            if "PREFETCHER: user" in lines[i]:
                # print(lines[i:])
                user_stats = lines[i:]
                break

        user_results = {}

        for line in user_stats:
            for word in stop_words:
                if word in line:
                    test_results = line.split()
                    user_results[test_results[0]] = float(test_results[2])
                # print(test_results[0], user_results[test_results[0]])
        mean = harmonic_mean(user_results.values())
        means.append(mean)
        if mean > best_mean:
            best = [degree, depth, width]
            best_mean = mean
        print("Harmonic mean:", mean)

print("Best:", best)
print("Best mean:", best_mean)

plt.plot(nums, means)
plt.ylabel('speedup')
plt.show()
