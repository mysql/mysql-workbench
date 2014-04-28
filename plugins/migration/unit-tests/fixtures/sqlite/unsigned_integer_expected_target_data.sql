/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `UnsignedIntegerContainer` (
  `id` int(11) NOT NULL DEFAULT '0',
  `tint` tinyint(3) unsigned DEFAULT NULL,
  `sint` smallint(5) unsigned DEFAULT NULL,
  `mint` mediumint(8) unsigned DEFAULT NULL,
  `iint` int(10) unsigned DEFAULT NULL,
  `bint` bigint(20) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
INSERT INTO `UnsignedIntegerContainer` VALUES (1,NULL,NULL,NULL,NULL,NULL),(2,0,0,0,0,0),(3,127,32767,8388607,2147483647,9223372036854775807),(4,255,65535,16777215,4294967295,18446744073709551615);
