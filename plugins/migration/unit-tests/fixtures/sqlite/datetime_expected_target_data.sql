/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `TimeContainer` (
  `id` int(11) NOT NULL DEFAULT '0',
  `date_data` date DEFAULT NULL,
  `time_data` time DEFAULT NULL,
  `timestamp_data` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
INSERT INTO `TimeContainer` VALUES (1,NULL,NULL,NULL),(2,'2013-12-30','23:59:59','2013-12-30 23:59:59'),(3,'2012-01-15','00:00:01','2012-01-15 00:00:01');
