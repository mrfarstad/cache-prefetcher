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


path = "stats/gdc"
depths = [2, 4, 8]
sizes = [16, 32, 64, 128, 256, 512, 1024]

best = []
best_mean = 0

for depth in depths:
    for size in sizes:
        with open(
                path + "/depth_" + str(depth) + "/ghb_ait_" + str(size) +
                "/stats.txt", "r") as file:
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
            if mean > best_mean:
                best = [depth, size]
                best_mean = mean
            print("Harmonic mean:", mean)

print("Best:", best)
print("Best mean:", best_mean)
