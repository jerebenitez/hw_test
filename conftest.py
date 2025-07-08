import serial
import pytest


def pytest_addoption(parser):
    parser.addoption("--port", action="store")
    parser.addoption("--baudrate", action="store", type=int, default=9600)


@pytest.fixture(scope="session")
def device(request):
    port = request.config.getoption("--port")
    baudrate = request.config.getoption("--baudrate")

    ser = serial.Serial(port=port, baudrate=baudrate, timeout=100)
    yield ser

    ser.close()
