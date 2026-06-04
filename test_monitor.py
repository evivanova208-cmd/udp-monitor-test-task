import subprocess
import time
import socket
import pytest
import grpc
import monitor_pb2
import monitor_pb2_grpc

PROGRAM_PATH = "./build/grpc_udp_monitor"
GRPC_PORT = 2031
UDP_PORT = 2032
UDP_HOST = "127.0.0.1"
TIMEOUT_READY = 5.0
TIMEOUT_SHUTDOWN = 2.0

@pytest.fixture(scope="function")
def program():
    proc = subprocess.Popen([PROGRAM_PATH])
    time.sleep(0.5)

    channel = grpc.insecure_channel(f"{UDP_HOST}:{GRPC_PORT}")
    stub = monitor_pb2_grpc.MonitorServiceStub(channel)

    start = time.time()
    while time.time() - start < TIMEOUT_READY:
        try:
            resp = stub.IsReady(monitor_pb2.Empty())
            if resp.is_ready:
                break
        except grpc.RpcError:
            pass
        time.sleep(0.1)
    else:
        proc.terminate()
        pytest.fail("Program didn't become ready")

    yield stub, proc

    proc.terminate()
    try:
        proc.wait(timeout=TIMEOUT_SHUTDOWN)
    except subprocess.TimeoutExpired:
        proc.kill()
    # Проверяем, что процесс завершился (любой код, кроме None)
    assert proc.returncode is not None, "Process did not terminate"

def send_udp(data):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(data.encode(), (UDP_HOST, UDP_PORT))
    sock.close()

def test_empty(program):
    stub, _ = program
    stats = stub.GetUdpStatistics(monitor_pb2.Empty())
    assert stats.packets == 0
    assert stats.aBytes == 0

def test_one_datagram_no_A(program):
    stub, _ = program
    send_udp("BCD")
    time.sleep(0.1)
    stats = stub.GetUdpStatistics(monitor_pb2.Empty())
    assert stats.packets == 1
    assert stats.aBytes == 0

def test_one_datagram_with_A(program):
    stub, _ = program
    send_udp("AAAB")
    time.sleep(0.1)
    stats = stub.GetUdpStatistics(monitor_pb2.Empty())
    assert stats.packets == 1
    assert stats.aBytes == 3

def test_multiple_datagrams(program):
    stub, _ = program
    send_udp("A")
    send_udp("BCA")
    send_udp("AAA")
    time.sleep(0.2)
    stats = stub.GetUdpStatistics(monitor_pb2.Empty())
    assert stats.packets == 3
    assert stats.aBytes == 5
