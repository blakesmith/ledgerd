# LedgerD

## Client Example

    # ledgerd_client ping

    # ledgerd_client --topic my_topic --partition_count 1 --command open_topic
    => OPENED: my_topic

    # ledgerd_client --topic my_topic --partition 0 --data hello --command write_partition
    => ID: 43
    
    # ledgerd_client --topic my_topic --partition 0 --start 43 --command read_partition --nmessages 1
    => hello
