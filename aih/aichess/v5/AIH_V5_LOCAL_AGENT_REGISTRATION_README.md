# AIH v5 Local Agent Registration Database

Generated: 2026-08-15 11:12:44 MDT

Source run:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624
```

Stable files:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LOCAL_AGENT_REGISTRATION_VERIFIED.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LOCAL_AGENT_REGISTRATION_FAILURES.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LOCAL_AGENT_REGISTRATION_COMM_SETTINGS.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LOCAL_AGENT_REGISTRATION_SUMMARY.csv
```

Current status:

```text
verified_pass=15
registration_fail=4
```

Use the communication settings in AIH v5 registration:

```bash
AIH_V5_REGISTRATION_COMM_SETTINGS_CSV=/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LOCAL_AGENT_REGISTRATION_COMM_SETTINGS.csv \
AIH_V5_REGISTRATION_MODE=game \
AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT=0 \
./aih_v5.sh --registration-only --no-open --registration-forward
```

Policy:

- Local agent registration is a separate AIH v5 phase.
- Verified agents are listed in the verified CSV.
- Communication settings are stored separately and can be consumed by AIH v5 registration.
- Full raw response JSON paths are preserved for audit.
- response remains the official agent answer; thinking is diagnostic unless a policy explicitly says otherwise.
