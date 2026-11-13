"""Regression checks for the vendored Flight MAVLink dialect."""

from __future__ import annotations

import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
FLIGHT_XML = REPO_ROOT / "src" / "MAVLink" / "flight.xml"
DIALECT_PATCH = REPO_ROOT / "src" / "MAVLink" / "mavlink-flight-dialect.patch"

EXPECTED_MESSAGES = [
    (43000, "FCU_OPERATION_STATUS"),
    (43001, "FCU_NAV_STATUS"),
    (43002, "FCU_SENSOR_HEALTH"),
    (43003, "FCU_CONTROL_STATUS"),
    (43004, "FCU_ACTUATOR_STATUS"),
    (43005, "FCU_LINK_STATUS"),
    (43006, "FCU_STORAGE_STATUS"),
    (43007, "FCU_RESOURCE_STATUS"),
    (43008, "FCU_TIME_STATUS"),
    (43009, "FCU_EVENT"),
    (43010, "FCU_PREARM_STATUS"),
    (43011, "FCU_RC_INPUT_STATUS"),
    (43012, "FCU_BUILD_INFO"),
]


def test_fcu_dialect_message_registry_is_contiguous_and_unique() -> None:
    root = ET.parse(FLIGHT_XML).getroot()
    messages = root.find("messages")
    assert messages is not None

    actual = [
        (int(message.attrib["id"]), message.attrib["name"])
        for message in messages.findall("message")
    ]
    assert actual == EXPECTED_MESSAGES

    for message in messages.findall("message"):
        fields = [field.attrib["name"] for field in message.findall("field")]
        assert len(fields) == len(set(fields)), message.attrib["name"]


def test_mavlink_patch_embeds_the_exact_local_fcu_contract() -> None:
    local_xml = FLIGHT_XML.read_text(encoding="utf-8").rstrip("\n") + "\n"
    patch_lines = DIALECT_PATCH.read_text(encoding="utf-8").splitlines()
    marker = "+++ b/message_definitions/v1.0/flight.xml"
    assert marker in patch_lines
    assert "+  <include>flight.xml</include>" in patch_lines

    start = patch_lines.index(marker)
    embedded_lines: list[str] = []
    for line in patch_lines[start + 1 :]:
        if line.startswith("--- "):
            break
        if line.startswith("+"):
            embedded_lines.append(line[1:])

    embedded_xml = "\n".join(embedded_lines) + "\n"
    assert embedded_xml == local_xml
