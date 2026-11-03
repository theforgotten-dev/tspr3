# Seguridad

TSPR3 es software experimental. Solo la última versión publicada recibe
correcciones de seguridad. Reporta problemas de forma privada mediante
[GitHub Security Advisories](https://github.com/theforgotten-dev/tspr3/security/advisories/new).
No publiques tokens, contraseñas, claves privadas, ROMs ni datos personales en issues.

Los secretos de release se guardan fuera del repositorio, cifrados con DPAPI para
el usuario Windows actual. Un secreto expuesto debe revocarse; borrarlo del Git o
del chat no lo vuelve seguro. El proyecto nunca necesita la clave GPG privada en CI.

Los artefactos Maven incluyen fuentes, Javadoc, firmas y SBOM CycloneDX. Los tags
de nuevas releases deben estar firmados y el JAR público debe coincidir por SHA-256
con el artefacto validado localmente.
