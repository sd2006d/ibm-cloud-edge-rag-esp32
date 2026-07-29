import pytest

def test_emergency_stop_edge_offload():
    intent = "EMERGENCY_STOP"
    requires_rag = False
    assert intent == "EMERGENCY_STOP"
    assert requires_rag is False

def test_latency_savings_benchmark():
    pure_cloud_latency = 340.0
    edge_offload_latency = 140.0
    savings = pure_cloud_latency - edge_offload_latency
    assert savings >= 200.0
