# LedgerD

## Client Example

    # ledgerd_client ping

    # ledgerd_client --topic my_topic --partition_count 1 open_topic
    => OPENED: my_topic

    # ledgerd_client --topic my_topic --partition 0 --data hello write_partition
    => ID: 43
    
    # ledgerd_client --topic my_topic --partition 0 --start 43 read_partition
    => hello
