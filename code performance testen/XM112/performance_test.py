from acconeer.exptool import configs, utils
from acconeer.exptool.clients import SocketClient, SPIClient, UARTClient
import time
import statistics

MAX_RANGE_VL53L0X_IN_CM = 200
seconds_for_starting_up = 60
seconds_running_performance_test = 300
seconds_to_measure = seconds_for_starting_up + seconds_running_performance_test
start = time.time()
measures = []
counted_measures_each_centimeter = [0] * MAX_RANGE_VL53L0X_IN_CM

# Print statistics based on saved measurements.
def print_result():
    print("Amount measures that were taken: {} \n".format(len(measures)))
    print("Avarage in meters: {} \n".format(sum(measures) / len(measures)))
    print("Standard deviation in meters: {} \n".format(statistics.pstdev(measures)))
    print("Median in meters: {} \n".format(statistics.median(measures)))
    print("Shortest measurement in meters: {} \n".format(min(measures)))
    print("Longest measurement in meters: {} \n".format(max(measures)))

    for i in range(MAX_RANGE_VL53L0X_IN_CM):
        if counted_measures_each_centimeter[i] != 0:
            print("{} times a measure of {} cm has been measured.".format(counted_measures_each_centimeter[i], i))

# Run test to get performance statistics about the xm112 radar chip.
def run_test(measurement_in_meters):

    if measurement_in_meters > 999:
        return False

    seconds_current = (time.time() - start)
    is_startup_done = seconds_current > seconds_for_starting_up

    if is_startup_done:
        is_performance_test_done = (time.time() - start) > seconds_to_measure
        if is_performance_test_done:
            print_result()
            return True
        else:
            print("[{}] : {}  meters. Time left : {} \r".format(len(measures), measurement_in_meters, seconds_to_measure - seconds_current))
            measures.append(measurement_in_meters)
            counted_measures_each_centimeter[round(measurement_in_meters * 100)] += 1
    else:
        if not is_startup_done:
            print("Seconds before starting : {}. Current measurement right now : {} meters \r".format(seconds_for_starting_up - seconds_current, measurement_in_meters))
    return False

def main():
    args = utils.ExampleArgumentParser().parse_args()
    utils.config_logging(args)

    if args.socket_addr:
        client = SocketClient(args.socket_addr)
    elif args.spi:
        client = SPIClient()
    else:
        port = args.serial_port or utils.autodetect_serial_port()
        client = UARTClient(port)

    # Normally when using a single sensor, get_next will return
    # (info, data). When using mulitple sensors, get_next will return
    # lists of info and data for each sensor, i.e. ([info], [data]).
    # This is commonly called squeezing. To disable squeezing, making
    # get_next _always_ return lists, set:
    # client.squeeze = False

    config = configs.EnvelopeServiceConfig()
    config.sensor = args.sensors
    config.range_interval = [0.4, 0.7]
    config.update_rate = 20
    config.profile = config.Profile.PROFILE_2
    config.hw_accelerated_average_samples = 20
    config.downsampling_factor = 2
    session_info = client.setup_session(config)
    print("Session info:\n", session_info, "\n")

    # Now would be the time to set up plotting, signal processing, etc.

    client.start_session()

    # Normally, hitting Ctrl-C will raise a KeyboardInterrupt which in
    # most cases immediately terminates the script. This often becomes
    # an issue when plotting and also doesn't allow us to disconnect
    # gracefully. Setting up an ExampleInterruptHandler will capture the
    # keyboard interrupt signal so that a KeyboardInterrupt isn't raised
    # and we can take care of the signal ourselves. In case you get
    # impatient, hitting Ctrl-C a couple of more times will raise a
    # KeyboardInterrupt which hopefully terminates the script.
    interrupt_handler = utils.ExampleInterruptHandler()
    print("Press Ctrl-C to end session\n")

    is_performance_test_done = False

    while not interrupt_handler.got_signal:
        info, data = client.get_next()
        my_data = list(data)
        # Get the index of the highest measured signal strength from the list.
        index_max_signal_strength = my_data.index(max(my_data))
        # Calculate the distance of the highest measured signal strength in meters.
        if max(my_data) > 400:
            distance_of_strongest_signal_strength = 0.4 + (index_max_signal_strength * session_info["step_length_m"])
        else:
            distance_of_strongest_signal_strength = 1000
            print("No strong signal")
        #print(distance_of_strongest_signal_strength)
        #print("meters, signal strength : ")
        #print(index_max_signal_strength)
        #print("\r")
        print(max(my_data))
        # Run a performance test.
        if is_performance_test_done is False:
           is_performance_test_done = run_test(distance_of_strongest_signal_strength)
    print("Disconnecting...")
    client.disconnect()

if __name__ == "__main__":
    main()