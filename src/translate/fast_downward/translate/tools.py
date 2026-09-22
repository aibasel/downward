def get_peak_memory_in_kb():
    try:
        # This will only work on Linux systems.
        with open("/proc/self/status") as status_file:
            for line in status_file:
                parts = line.split()
                if parts[0] == "VmPeak:":
                    return int(parts[1])
    except OSError:
        pass
    raise Warning("warning: could not determine peak memory")
