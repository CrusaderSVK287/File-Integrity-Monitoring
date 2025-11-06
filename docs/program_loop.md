# Init
- Verifies config and password
- Reads config
- Loads modules

# Main loop
Every x seconds:
- For every file in config:
    - Compute a hash
    - Retrieve a hash from database manager
        - If does not exist, store the hash in the database and continue to next file
    - Verify that the hashes are valid
        - If not, send a notify alert to the mailing manager
    - Continue with next file until all files done
    - Wait x seconds
