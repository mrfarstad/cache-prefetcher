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


path = "depth_stats"
depths = [1, 2, 4, 6, 8, 10]

best = 0
best_mean = 0

for depth in depths:
    with open(path + "/depth_" + str(depth) + "/stats.txt", "r") as file:
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
            best = depth
            best_mean = mean
        print("Harmonic mean:", mean)

print("Best depth:", best)
print("Best degree:", best_mean)
print("Best mean:", best_mean)
