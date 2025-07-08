import re


def test_get_status(device):
    device.reset_input_buffer()
    device.write(b'STATUS\n')

    response = device.readline().decode().strip()
    assert response == "0"


def test_get_temp(device):
    pattern = r"-?[0-9]{1,2}\.[0-9]"

    device.reset_input_buffer()
    device.write(b'GET_TEMP\n')

    response = device.readline().decode().strip()
    assert re.search(pattern, response)


def test_invalid_command(device):
    device.reset_input_buffer()
    device.write(b'INVALID\n')

    response = device.readline().decode().strip()
    assert response == "ERROR"


def test_set_led(device):
    device.reset_input_buffer()
    device.write(b'SET_LED 9\n')
    response = device.readline().decode().strip()
    assert response == "OK"

    device.write(b'STATUS\n')
    response = device.readline().decode().strip()
    assert response == "9"


def test_reset(device):
    device.reset_input_buffer()
    device.write(b'SET_LED 9\n')
    response = device.readline().decode().strip()
    assert response == "OK"
    device.write(b'RESET\n')
    device.write(b'STATUS\n')
    response = device.readline().decode().strip()
    assert response == "0"


def test_set_led_bigger_than_8bits(device):
    device.reset_input_buffer()
    device.write(b'RESET\n')
    device.write(b'SET_LED 500\n')

    response = device.readline().decode().strip()
    assert response == "ERROR"

    device.write(b'STATUS\n')

    response = device.readline().decode().strip()
    assert response == "0"


def test_set_led_invalid_input(device):
    device.reset_input_buffer()
    device.write(b'RESET\n')
    device.write(b'SET_LED B\n')

    response = device.readline().decode().strip()
    assert response == "ERROR"

    device.write(b'STATUS\n')

    response = device.readline().decode().strip()
    assert response == "0"


def test_led_toggle(device):
    device.reset_input_buffer()
    device.write(b'RESET\n')
    device.write(b'SET_LED 1\n')
    response = device.readline().decode().strip()
    assert response == "OK"

    device.write(b'STATUS\n')
    response = device.readline().decode().strip()
    assert response == "1"

    device.reset_input_buffer()
    device.reset_output_buffer()
    device.write(b'SET_LED 1\n')
    response = device.readline().decode().strip()
    assert response == "OK"

    device.write(b'STATUS\n')
    response = device.readline().decode().strip()
    assert response == "0"


def test_multiple_cmds(device):
    device.reset_input_buffer()
    device.write(b'STATUS STATUS\n')
    response = device.readline().decode().strip()
    print(response)
    assert response == "0"

    print(response)
    response = device.readline().decode().strip()
    assert response == "0"


def test_get_temp_values(device):
    for i in range(10000):
        device.reset_input_buffer()
        device.write(b'GET_TEMP\n')
        response = float(device.readline().decode().strip())

        assert -10.0 <= response <= 45.0


def test_long_msg(device):
    device.reset_input_buffer()
    device.write(b'A'*1024 + b'\n')

    response = device.readline().decode().strip()
    assert response == "ERROR"
