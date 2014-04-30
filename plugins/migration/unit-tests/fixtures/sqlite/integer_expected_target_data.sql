/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `IntegerContainer` (
  `id` int(11) NOT NULL DEFAULT '0',
  `tint` tinyint(4) DEFAULT NULL,
  `sint` smallint(6) DEFAULT NULL,
  `mint` mediumint(9) DEFAULT NULL,
  `iint` int(11) DEFAULT NULL,
  `bint` bigint(20) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
INSERT INTO `IntegerContainer` VALUES (1,NULL,NULL,NULL,NULL,NULL),(2,0,0,0,0,0),(3,-128,-32768,-8388608,-2147483648,-9223372036854775808),(4,127,32767,8388607,2147483647,9223372036854775807);
