# Class 2 IT Tools And Procedures Candidate Tests

Created: 20260715_1939MDT

## Purpose

Seed a starter set of Class 2 AIH tests for common IT tools, procedures, and tech-support workflows.

These are Class 2 tests because the target failure is provenance/workflow-history hallucination: whether an agent can track what it did, what evidence it used, what step it reached, what it verified, and what remains unverified. These are not yet Class 3 certification-knowledge exams.

## Subject Field

```text
class = class_2_provenance_workflow
subject_field = information_technology_tools_and_support
source_context = synthetic_or_public_safe_workflow
```

## Candidate Areas

### Network Firewall And Router Preparation

- Prepare and verify firewall settings on Cisco business-class routers.
- Prepare and verify firewall settings on home/office LAN wireless-Ethernet routers.
- Record source configuration, proposed changes, applied changes, rollback point, and verification evidence.
- Detect hallucinations about rules that were proposed but not applied, ports that were not verified, or device models/features not present.

### Desktop And Business Phone Support

- Common desktop OS support workflows for Windows and Linux.
- Business phone OS/application setup and troubleshooting.
- Application install, update, removal, and configuration verification.
- Detect hallucinations about completed installs, changed settings, user permissions, or unresolved restart/update states.

### Certification-Aligned Support Domains

- A+ style hardware/software support tasks.
- SMP-style support/process tasks if the source packet defines the acronym and scope.
- Azure administration/support tasks.
- Windows and Linux certification-aligned setup, repair, and verification workflows.
- Keep certification content source-bound if later promoted into Class 3; keep workflow/provenance tracking here as Class 2.

## Certification Grouping Layer

Candidate tests can be grouped by common certification families. This grouping is a planning and reporting layer, not an exclusive taxonomy. A single workflow may map to more than one certification family.

Recommended metadata:

```text
certification_group = <group>
certification_level = <entry | associate | professional | specialist | unspecified>
vendor_context = <vendor-neutral | Microsoft | Cisco | Linux | cloud | mixed>
workflow_domain = <networking | endpoint | server | cloud | security | hardware | support_process | mixed>
```

Starter certification groups:

- CompTIA A+ / entry hardware and endpoint support
- CompTIA Network+ / TCP/IP, LAN, cabling, routing, switching, wireless basics
- CompTIA Security+ / firewall, access-control, security-baseline, verification
- Cisco CCST/CCNA-style small-business routing, switching, wireless, and firewall workflows
- Microsoft Windows desktop/admin support
- Microsoft Azure fundamentals/administrator support
- Linux administration / Linux+ style workflows
- Server administration / web server / SQL server support
- Cloud fundamentals / cloud server setup and verification
- ITIL/help-desk/service-management support process
- Hardware service / rack, power, thermal, peripheral, RAID, and repair workflows

Use certification grouping to compare similar workflow/provenance failures across domains. Do not treat a Class 2 certification group as proof that the test reproduces or replaces a certification exam.

### TCP/IP And LAN Configuration

- TCP/IP configuration and verification.
- DHCP/static IP changes.
- DNS/gateway/subnet checks.
- Wi-Fi connectivity setup and repair.
- Cable and four-wire Ethernet checks.
- Detect hallucinations about connectivity, interface state, routing, DNS resolution, or test commands not actually run.

### Peripheral And Hardware Install/Removal

- Peripheral install and removal.
- Driver install/removal.
- RAID configuration and repair.
- Rack mounting.
- Power-supply wiring.
- Power-supply selection.
- Thermal coolant selection and replacement.
- Detect hallucinations about model compatibility, installed parts, wiring state, drive health, thermal status, or completed physical verification.

### Cross-OS Filesystem And Network Sharing

- Make OS partitions read/writable from a different OS through a network connection.
- Configure and verify Samba/SMB access.
- Track mount state, share permissions, filesystem permissions, read/write tests, and rollback.
- Detect hallucinations about access rights, successful write tests, authentication mode, or network path.

### Server Configuration And Remote Client Interfaces

- Web server configuration.
- SQL server configuration.
- Remote client connectivity to web and SQL services.
- Interface checks between application, web server, SQL server, firewall, DNS, and client.
- Detect hallucinations about service status, open ports, database credentials, schema availability, or client-side verification.

### Cloud Server Setup And Verification

- Cloud server provisioning.
- Firewall/security-group setup.
- SSH/RDP or management access verification.
- Web/SQL/application service exposure.
- Backup/snapshot/rollback confirmation.
- Detect hallucinations about cloud region, instance state, public/private IP, firewall exposure, credential setup, or verification evidence.

## Starter Test Pattern

Each candidate Class 2 IT test should include:

```text
initial_state
requested_task
allowed_tools
agent_claimed_steps
actual_event_log
verification_artifacts
unverified_items
rollback_or_recovery_state
scoring_rules
```

The scoring question is not only whether the IT task was correct. The Class 2 scoring question is whether the agent accurately reports provenance, workflow state, verification state, uncertainty, and remaining risk.
