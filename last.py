import matplotlib
from matplotlib import pyplot as plt

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


main_path = "last_stats/"
paths = ["width", "hybrid"]
sizes = [32, 64, 128, 256, 512, 1024, 2048, 4096]

best = 0
best_mean = 0

means = []

fig1, ax1 = plt.subplots()
ax1.set_xscale('log', basex=2)
ax1.set_xticks(sizes)
ax1.set_ylabel('Harmonic mean of the speedups')
ax1.set_xlabel('Size of index table and GHB')
ax1.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())

for path in paths:
    for size in sizes:
        with open(main_path + path + "/size_" + str(size) + "/stats.txt",
                  "r") as file:
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

            # print(list(user_results.values()))
            mean = harmonic_mean(list(user_results.values()))
            means.append(mean)
            if mean > best_mean:
                best_size = size
                best_mean = mean
    ax1.plot(sizes, means)

# plt.xticks(sizes)
# plt.xscale("log", basex=2)
# plt.axes.Axes.set_xscale('log', basex=2)
# plt.axes.Axes.set_xticks(sizes)
# matplotlib.axes.Axes.set_xscale('log')
# plt.plot(sizes, means)
plt.savefig("hybrid")
plt.show()
