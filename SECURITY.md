# Security Policy

## Supported Versions
The latest `main` branch and the most recent tagged release receive security attention.

## Reporting a Vulnerability
**Please do NOT open a public issue for security reports.**

- Use **GitHub Private Vulnerability Reporting / Security Advisories** for this repository (preferred).
- Alternatively, email the maintainer: **<add-your-email-here>**.

What to include:
- A clear description of the issue and potential impact.
- Reproduction steps or a minimal PoC (if possible).
- Affected version/commit and your OS/environment.

We aim to acknowledge reports within **7 days** and will coordinate a fix and a disclosure timeline. If appropriate, we may publish a security advisory and a patched release.

## Scope Notes
This project generates Windows batch files. Be mindful that:
- Running elevated processes (UAC) has inherent risks.
- Do not include secrets in paths/arguments.
- Review generated `.bat` before sharing or executing in sensitive environments.