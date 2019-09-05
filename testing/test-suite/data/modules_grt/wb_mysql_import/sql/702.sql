/*
SQLyog Community Edition- MySQL GUI v6.0 Beta 5 
Host - 5.1.22-rc-community : Database - sakila
*********************************************************************
Server version : 5.1.22-rc-community
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

create database if not exists `sakila`;

USE `sakila`;

/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

/*Table structure for table `actor` */

DROP TABLE IF EXISTS `actor`;

CREATE TABLE `actor` (
  `actor_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `first_name` varchar(45) NOT NULL,
  `last_name` varchar(45) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`),
  KEY `idx_actor_last_name` (`last_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `actor` */

/*Table structure for table `actor_info` */

DROP TABLE IF EXISTS `actor_info`;

/*!50001 DROP VIEW IF EXISTS `actor_info` */;
/*!50001 DROP TABLE IF EXISTS `actor_info` */;

/*!50001 CREATE TABLE `actor_info` (
  `actor_id` smallint(5) unsigned NOT NULL DEFAULT '0',
  `first_name` varchar(45) CHARACTER SET utf8 NOT NULL,
  `last_name` varchar(45) CHARACTER SET utf8 NOT NULL,
  `film_info` varchar(342) CHARACTER SET utf8 DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `address` */

DROP TABLE IF EXISTS `address`;

CREATE TABLE `address` (
  `address_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `address` varchar(50) NOT NULL,
  `address2` varchar(50) DEFAULT NULL,
  `district` varchar(20) NOT NULL,
  `city_id` smallint(5) unsigned NOT NULL,
  `postal_code` varchar(10) DEFAULT NULL,
  `phone` varchar(20) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`address_id`),
  KEY `idx_fk_city_id` (`city_id`),
  CONSTRAINT `fk_address_city` FOREIGN KEY (`city_id`) REFERENCES `city` (`city_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `address` */

/*Table structure for table `category` */

DROP TABLE IF EXISTS `category`;

CREATE TABLE `category` (
  `category_id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(25) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`category_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `category` */

/*Table structure for table `city` */

DROP TABLE IF EXISTS `city`;

CREATE TABLE `city` (
  `city_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `city` varchar(50) NOT NULL,
  `country_id` smallint(5) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`city_id`),
  KEY `idx_fk_country_id` (`country_id`),
  CONSTRAINT `fk_city_country` FOREIGN KEY (`country_id`) REFERENCES `country` (`country_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `city` */

/*Table structure for table `country` */

DROP TABLE IF EXISTS `country`;

CREATE TABLE `country` (
  `country_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `country` varchar(50) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`country_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `country` */

/*Table structure for table `customer` */

DROP TABLE IF EXISTS `customer`;

CREATE TABLE `customer` (
  `customer_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `store_id` tinyint(3) unsigned NOT NULL,
  `first_name` varchar(45) NOT NULL,
  `last_name` varchar(45) NOT NULL,
  `email` varchar(50) DEFAULT NULL,
  `address_id` smallint(5) unsigned NOT NULL,
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `create_date` datetime NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`customer_id`),
  KEY `idx_fk_store_id` (`store_id`),
  KEY `idx_fk_address_id` (`address_id`),
  KEY `idx_last_name` (`last_name`),
  CONSTRAINT `fk_customer_address` FOREIGN KEY (`address_id`) REFERENCES `address` (`address_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_customer_store` FOREIGN KEY (`store_id`) REFERENCES `store` (`store_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `customer` */

/*Table structure for table `customer_list` */

DROP TABLE IF EXISTS `customer_list`;

/*!50001 DROP VIEW IF EXISTS `customer_list` */;
/*!50001 DROP TABLE IF EXISTS `customer_list` */;

/*!50001 CREATE TABLE `customer_list` (
  `ID` smallint(5) unsigned NOT NULL DEFAULT '0',
  `name` varchar(91) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `address` varchar(50) CHARACTER SET utf8 NOT NULL,
  `zip code` varchar(10) CHARACTER SET utf8 DEFAULT NULL,
  `phone` varchar(20) CHARACTER SET utf8 NOT NULL,
  `city` varchar(50) CHARACTER SET utf8 NOT NULL,
  `country` varchar(50) CHARACTER SET utf8 NOT NULL,
  `notes` varchar(6) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `SID` tinyint(3) unsigned NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `film` */

DROP TABLE IF EXISTS `film`;

CREATE TABLE `film` (
  `film_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `title` varchar(255) NOT NULL,
  `description` text,
  `release_year` year(4) DEFAULT NULL,
  `language_id` tinyint(3) unsigned NOT NULL,
  `original_language_id` tinyint(3) unsigned DEFAULT NULL,
  `rental_duration` tinyint(3) unsigned NOT NULL DEFAULT '3',
  `rental_rate` decimal(4,2) NOT NULL DEFAULT '4.99',
  `length` smallint(5) unsigned DEFAULT NULL,
  `replacement_cost` decimal(5,2) NOT NULL DEFAULT '19.99',
  `rating` enum('G','PG','PG-13','R','NC-17') DEFAULT 'G',
  `special_features` set('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') DEFAULT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`),
  KEY `idx_title` (`title`),
  KEY `idx_fk_language_id` (`language_id`),
  KEY `idx_fk_original_language_id` (`original_language_id`),
  CONSTRAINT `fk_film_language` FOREIGN KEY (`language_id`) REFERENCES `language` (`language_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_film_language_original` FOREIGN KEY (`original_language_id`) REFERENCES `language` (`language_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `film` */

/*Trigger structure for table `film` */

DELIMITER $$

/*!50003 DROP TRIGGER IF EXISTS `ins_film` */$$

/*!50003 CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN
 INSERT INTO film_text (film_id, title, description)
 VALUES (new.film_id, new.title, new.description);
 END */$$


/*!50003 DROP TRIGGER IF EXISTS `upd_film` */$$

/*!50003 CREATE TRIGGER `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN
 IF (old.title != new.title) or (old.description != new.description)
 THEN
 UPDATE film_text
 SET title=new.title,
 description=new.description,
 film_id=new.film_id
 WHERE film_id=old.film_id;
 END IF;
 END */$$


/*!50003 DROP TRIGGER IF EXISTS `del_film` */$$

/*!50003 CREATE TRIGGER `del_film` AFTER DELETE ON `film` FOR EACH ROW BEGIN
 DELETE FROM film_text WHERE film_id = old.film_id;
 END */$$


DELIMITER ;

/*Table structure for table `film_actor` */

DROP TABLE IF EXISTS `film_actor`;

CREATE TABLE `film_actor` (
  `actor_id` smallint(5) unsigned NOT NULL,
  `film_id` smallint(5) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`,`film_id`),
  KEY `idx_fk_film_id` (`film_id`),
  CONSTRAINT `fk_film_actor_actor` FOREIGN KEY (`actor_id`) REFERENCES `actor` (`actor_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_film_actor_film` FOREIGN KEY (`film_id`) REFERENCES `film` (`film_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `film_actor` */

/*Table structure for table `film_category` */

DROP TABLE IF EXISTS `film_category`;

CREATE TABLE `film_category` (
  `film_id` smallint(5) unsigned NOT NULL,
  `category_id` tinyint(3) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`,`category_id`),
  KEY `fk_film_category_category` (`category_id`),
  CONSTRAINT `fk_film_category_film` FOREIGN KEY (`film_id`) REFERENCES `film` (`film_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_film_category_category` FOREIGN KEY (`category_id`) REFERENCES `category` (`category_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `film_category` */

/*Table structure for table `film_list` */

DROP TABLE IF EXISTS `film_list`;

/*!50001 DROP VIEW IF EXISTS `film_list` */;
/*!50001 DROP TABLE IF EXISTS `film_list` */;

/*!50001 CREATE TABLE `film_list` (
  `FID` smallint(5) unsigned DEFAULT '0',
  `title` varchar(255) CHARACTER SET utf8,
  `description` text CHARACTER SET utf8,
  `category` varchar(25) CHARACTER SET utf8 NOT NULL,
  `price` decimal(4,2) DEFAULT '4.99',
  `length` smallint(5) unsigned DEFAULT NULL,
  `rating` enum('G','PG','PG-13','R','NC-17') CHARACTER SET utf8 DEFAULT 'G',
  `actors` varchar(342) CHARACTER SET utf8 DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `film_text` */

DROP TABLE IF EXISTS `film_text`;

CREATE TABLE `film_text` (
  `film_id` smallint(6) NOT NULL,
  `title` varchar(255) NOT NULL,
  `description` text,
  PRIMARY KEY (`film_id`),
  FULLTEXT KEY `idx_title_description` (`title`,`description`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `film_text` */

/*Table structure for table `inventory` */

DROP TABLE IF EXISTS `inventory`;

CREATE TABLE `inventory` (
  `inventory_id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `film_id` smallint(5) unsigned NOT NULL,
  `store_id` tinyint(3) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`inventory_id`),
  KEY `idx_fk_film_id` (`film_id`),
  KEY `idx_store_id_film_id` (`store_id`,`film_id`),
  CONSTRAINT `fk_inventory_store` FOREIGN KEY (`store_id`) REFERENCES `store` (`store_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_inventory_film` FOREIGN KEY (`film_id`) REFERENCES `film` (`film_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `inventory` */

/*Table structure for table `language` */

DROP TABLE IF EXISTS `language`;

CREATE TABLE `language` (
  `language_id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `name` char(20) NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`language_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `language` */

/*Table structure for table `nicer_but_slower_film_list` */

DROP TABLE IF EXISTS `nicer_but_slower_film_list`;

/*!50001 DROP VIEW IF EXISTS `nicer_but_slower_film_list` */;
/*!50001 DROP TABLE IF EXISTS `nicer_but_slower_film_list` */;

/*!50001 CREATE TABLE `nicer_but_slower_film_list` (
  `FID` smallint(5) unsigned DEFAULT '0',
  `title` varchar(255) CHARACTER SET utf8,
  `description` text CHARACTER SET utf8,
  `category` varchar(25) CHARACTER SET utf8 NOT NULL,
  `price` decimal(4,2) DEFAULT '4.99',
  `length` smallint(5) unsigned DEFAULT NULL,
  `rating` enum('G','PG','PG-13','R','NC-17') CHARACTER SET utf8 DEFAULT 'G',
  `actors` varchar(342) CHARACTER SET utf8 DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `payment` */

DROP TABLE IF EXISTS `payment`;

CREATE TABLE `payment` (
  `payment_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `customer_id` smallint(5) unsigned NOT NULL,
  `staff_id` tinyint(3) unsigned NOT NULL,
  `rental_id` int(11) DEFAULT NULL,
  `amount` decimal(5,2) NOT NULL,
  `payment_date` datetime NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`payment_id`),
  KEY `idx_fk_staff_id` (`staff_id`),
  KEY `idx_fk_customer_id` (`customer_id`),
  KEY `fk_payment_rental` (`rental_id`),
  CONSTRAINT `fk_payment_rental` FOREIGN KEY (`rental_id`) REFERENCES `rental` (`rental_id`) ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT `fk_payment_customer` FOREIGN KEY (`customer_id`) REFERENCES `customer` (`customer_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_payment_staff` FOREIGN KEY (`staff_id`) REFERENCES `staff` (`staff_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `payment` */

/*Table structure for table `rental` */

DROP TABLE IF EXISTS `rental`;

CREATE TABLE `rental` (
  `rental_id` int(11) NOT NULL AUTO_INCREMENT,
  `rental_date` datetime NOT NULL,
  `inventory_id` mediumint(8) unsigned NOT NULL,
  `customer_id` smallint(5) unsigned NOT NULL,
  `return_date` datetime DEFAULT NULL,
  `staff_id` tinyint(3) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`rental_id`),
  UNIQUE KEY `rental_date` (`rental_date`,`inventory_id`,`customer_id`),
  KEY `idx_fk_inventory_id` (`inventory_id`),
  KEY `idx_fk_customer_id` (`customer_id`),
  KEY `idx_fk_staff_id` (`staff_id`),
  CONSTRAINT `fk_rental_staff` FOREIGN KEY (`staff_id`) REFERENCES `staff` (`staff_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_rental_inventory` FOREIGN KEY (`inventory_id`) REFERENCES `inventory` (`inventory_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_rental_customer` FOREIGN KEY (`customer_id`) REFERENCES `customer` (`customer_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `rental` */

/*Table structure for table `sales_by_film_category` */

DROP TABLE IF EXISTS `sales_by_film_category`;

/*!50001 DROP VIEW IF EXISTS `sales_by_film_category` */;
/*!50001 DROP TABLE IF EXISTS `sales_by_film_category` */;

/*!50001 CREATE TABLE `sales_by_film_category` (
  `category` varchar(25) CHARACTER SET utf8 NOT NULL,
  `total_sales` decimal(27,2) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `sales_by_store` */

DROP TABLE IF EXISTS `sales_by_store`;

/*!50001 DROP VIEW IF EXISTS `sales_by_store` */;
/*!50001 DROP TABLE IF EXISTS `sales_by_store` */;

/*!50001 CREATE TABLE `sales_by_store` (
  `store` varchar(101) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `manager` varchar(91) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `total_sales` decimal(27,2) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `staff` */

DROP TABLE IF EXISTS `staff`;

CREATE TABLE `staff` (
  `staff_id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `first_name` varchar(45) NOT NULL,
  `last_name` varchar(45) NOT NULL,
  `address_id` smallint(5) unsigned NOT NULL,
  `picture` blob,
  `email` varchar(50) DEFAULT NULL,
  `store_id` tinyint(3) unsigned NOT NULL,
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `username` varchar(16) NOT NULL,
  `password` varchar(40) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`staff_id`),
  KEY `idx_fk_store_id` (`store_id`),
  KEY `idx_fk_address_id` (`address_id`),
  CONSTRAINT `fk_staff_store` FOREIGN KEY (`store_id`) REFERENCES `store` (`store_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_staff_address` FOREIGN KEY (`address_id`) REFERENCES `address` (`address_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `staff` */

/*Table structure for table `staff_list` */

DROP TABLE IF EXISTS `staff_list`;

/*!50001 DROP VIEW IF EXISTS `staff_list` */;
/*!50001 DROP TABLE IF EXISTS `staff_list` */;

/*!50001 CREATE TABLE `staff_list` (
  `ID` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `name` varchar(91) CHARACTER SET utf8 NOT NULL DEFAULT '',
  `address` varchar(50) CHARACTER SET utf8 NOT NULL,
  `zip code` varchar(10) CHARACTER SET utf8 DEFAULT NULL,
  `phone` varchar(20) CHARACTER SET utf8 NOT NULL,
  `city` varchar(50) CHARACTER SET utf8 NOT NULL,
  `country` varchar(50) CHARACTER SET utf8 NOT NULL,
  `SID` tinyint(3) unsigned NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 */;

/*Table structure for table `store` */

DROP TABLE IF EXISTS `store`;

CREATE TABLE `store` (
  `store_id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `manager_staff_id` tinyint(3) unsigned NOT NULL,
  `address_id` smallint(5) unsigned NOT NULL,
  `last_update` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`store_id`),
  UNIQUE KEY `idx_unique_manager` (`manager_staff_id`),
  KEY `idx_fk_address_id` (`address_id`),
  CONSTRAINT `fk_store_staff` FOREIGN KEY (`manager_staff_id`) REFERENCES `staff` (`staff_id`) ON UPDATE CASCADE,
  CONSTRAINT `fk_store_address` FOREIGN KEY (`address_id`) REFERENCES `address` (`address_id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*Data for the table `store` */

/*View structure for view actor_info */

/*!50001 DROP TABLE IF EXISTS `actor_info` */;
/*!50001 DROP VIEW IF EXISTS `actor_info` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY INVOKER VIEW `actor_info` AS select `a`.`actor_id` AS `actor_id`,`a`.`first_name` AS `first_name`,`a`.`last_name` AS `last_name`,group_concat(distinct concat(`c`.`name`,_utf8': ',(select group_concat(`f`.`title` order by `f`.`title` ASC separator ', ') AS `GROUP_CONCAT(f.title ORDER BY f.title SEPARATOR ', ')` from ((`film` `f` join `film_category` `fc` on((`f`.`film_id` = `fc`.`film_id`))) join `film_actor` `fa` on((`f`.`film_id` = `fa`.`film_id`))) where ((`fc`.`category_id` = `c`.`category_id`) and (`fa`.`actor_id` = `a`.`actor_id`)))) order by `c`.`name` ASC separator '; ') AS `film_info` from (((`actor` `a` left join `film_actor` `fa` on((`a`.`actor_id` = `fa`.`actor_id`))) left join `film_category` `fc` on((`fa`.`film_id` = `fc`.`film_id`))) left join `category` `c` on((`fc`.`category_id` = `c`.`category_id`))) group by `a`.`actor_id`,`a`.`first_name`,`a`.`last_name` */;

/*View structure for view customer_list */

/*!50001 DROP TABLE IF EXISTS `customer_list` */;
/*!50001 DROP VIEW IF EXISTS `customer_list` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `customer_list` AS select `cu`.`customer_id` AS `ID`,concat(`cu`.`first_name`,_utf8' ',`cu`.`last_name`) AS `name`,`a`.`address` AS `address`,`a`.`postal_code` AS `zip code`,`a`.`phone` AS `phone`,`city`.`city` AS `city`,`country`.`country` AS `country`,if(`cu`.`active`,_utf8'active',_utf8'') AS `notes`,`cu`.`store_id` AS `SID` from (((`customer` `cu` join `address` `a` on((`cu`.`address_id` = `a`.`address_id`))) join `city` on((`a`.`city_id` = `city`.`city_id`))) join `country` on((`city`.`country_id` = `country`.`country_id`))) */;

/*View structure for view film_list */

/*!50001 DROP TABLE IF EXISTS `film_list` */;
/*!50001 DROP VIEW IF EXISTS `film_list` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `film_list` AS select `film`.`film_id` AS `FID`,`film`.`title` AS `title`,`film`.`description` AS `description`,`category`.`name` AS `category`,`film`.`rental_rate` AS `price`,`film`.`length` AS `length`,`film`.`rating` AS `rating`,group_concat(concat(`actor`.`first_name`,_utf8' ',`actor`.`last_name`) separator ', ') AS `actors` from ((((`category` left join `film_category` on((`category`.`category_id` = `film_category`.`category_id`))) left join `film` on((`film_category`.`film_id` = `film`.`film_id`))) join `film_actor` on((`film`.`film_id` = `film_actor`.`film_id`))) join `actor` on((`film_actor`.`actor_id` = `actor`.`actor_id`))) group by `film`.`film_id` */;

/*View structure for view nicer_but_slower_film_list */

/*!50001 DROP TABLE IF EXISTS `nicer_but_slower_film_list` */;
/*!50001 DROP VIEW IF EXISTS `nicer_but_slower_film_list` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `nicer_but_slower_film_list` AS select `film`.`film_id` AS `FID`,`film`.`title` AS `title`,`film`.`description` AS `description`,`category`.`name` AS `category`,`film`.`rental_rate` AS `price`,`film`.`length` AS `length`,`film`.`rating` AS `rating`,group_concat(concat(concat(ucase(substr(`actor`.`first_name`,1,1)),lcase(substr(`actor`.`first_name`,2,length(`actor`.`first_name`))),_utf8' ',concat(ucase(substr(`actor`.`last_name`,1,1)),lcase(substr(`actor`.`last_name`,2,length(`actor`.`last_name`)))))) separator ', ') AS `actors` from ((((`category` left join `film_category` on((`category`.`category_id` = `film_category`.`category_id`))) left join `film` on((`film_category`.`film_id` = `film`.`film_id`))) join `film_actor` on((`film`.`film_id` = `film_actor`.`film_id`))) join `actor` on((`film_actor`.`actor_id` = `actor`.`actor_id`))) group by `film`.`film_id` */;

/*View structure for view sales_by_film_category */

/*!50001 DROP TABLE IF EXISTS `sales_by_film_category` */;
/*!50001 DROP VIEW IF EXISTS `sales_by_film_category` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `sales_by_film_category` AS select `c`.`name` AS `category`,sum(`p`.`amount`) AS `total_sales` from (((((`payment` `p` join `rental` `r` on((`p`.`rental_id` = `r`.`rental_id`))) join `inventory` `i` on((`r`.`inventory_id` = `i`.`inventory_id`))) join `film` `f` on((`i`.`film_id` = `f`.`film_id`))) join `film_category` `fc` on((`f`.`film_id` = `fc`.`film_id`))) join `category` `c` on((`fc`.`category_id` = `c`.`category_id`))) group by `c`.`name` order by sum(`p`.`amount`) desc */;

/*View structure for view sales_by_store */

/*!50001 DROP TABLE IF EXISTS `sales_by_store` */;
/*!50001 DROP VIEW IF EXISTS `sales_by_store` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `sales_by_store` AS select concat(`c`.`city`,_utf8',',`cy`.`country`) AS `store`,concat(`m`.`first_name`,_utf8' ',`m`.`last_name`) AS `manager`,sum(`p`.`amount`) AS `total_sales` from (((((((`payment` `p` join `rental` `r` on((`p`.`rental_id` = `r`.`rental_id`))) join `inventory` `i` on((`r`.`inventory_id` = `i`.`inventory_id`))) join `store` `s` on((`i`.`store_id` = `s`.`store_id`))) join `address` `a` on((`s`.`address_id` = `a`.`address_id`))) join `city` `c` on((`a`.`city_id` = `c`.`city_id`))) join `country` `cy` on((`c`.`country_id` = `cy`.`country_id`))) join `staff` `m` on((`s`.`manager_staff_id` = `m`.`staff_id`))) group by `s`.`store_id` order by `cy`.`country`,`c`.`city` */;

/*View structure for view staff_list */

/*!50001 DROP TABLE IF EXISTS `staff_list` */;
/*!50001 DROP VIEW IF EXISTS `staff_list` */;

/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `staff_list` AS select `s`.`staff_id` AS `ID`,concat(`s`.`first_name`,_utf8' ',`s`.`last_name`) AS `name`,`a`.`address` AS `address`,`a`.`postal_code` AS `zip code`,`a`.`phone` AS `phone`,`city`.`city` AS `city`,`country`.`country` AS `country`,`s`.`store_id` AS `SID` from (((`staff` `s` join `address` `a` on((`s`.`address_id` = `a`.`address_id`))) join `city` on((`a`.`city_id` = `city`.`city_id`))) join `country` on((`city`.`country_id` = `country`.`country_id`))) */;

/* Procedure structure for procedure `film_in_stock` */

/*!50003 DROP PROCEDURE IF EXISTS  `film_in_stock` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` PROCEDURE `film_in_stock`(IN p_film_id INT, IN p_store_id INT, OUT p_film_count INT)
    READS SQL DATA
BEGIN
 SELECT inventory_id
 FROM inventory
 WHERE film_id = p_film_id
 AND store_id = p_store_id
 AND inventory_in_stock(inventory_id);
 SELECT FOUND_ROWS() INTO p_film_count;
 END */$$
DELIMITER ;

/* Procedure structure for procedure `film_not_in_stock` */

/*!50003 DROP PROCEDURE IF EXISTS  `film_not_in_stock` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` PROCEDURE `film_not_in_stock`(IN p_film_id INT, IN p_store_id INT, OUT p_film_count INT)
    READS SQL DATA
BEGIN
 SELECT inventory_id
 FROM inventory
 WHERE film_id = p_film_id
 AND store_id = p_store_id
 AND NOT inventory_in_stock(inventory_id);
 SELECT FOUND_ROWS() INTO p_film_count;
 END */$$
DELIMITER ;

/* Procedure structure for procedure `rewards_report` */

/*!50003 DROP PROCEDURE IF EXISTS  `rewards_report` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` PROCEDURE `rewards_report`(
 IN min_monthly_purchases TINYINT UNSIGNED
 , IN min_dollar_amount_purchased DECIMAL(10,2) UNSIGNED
 , OUT count_rewardees INT
 )
    READS SQL DATA
    COMMENT 'Provides a customizable report on best customers'
proc: BEGIN
 DECLARE last_month_start DATE;
 DECLARE last_month_end DATE;
 IF min_monthly_purchases = 0 THEN
 SELECT 'Minimum monthly purchases parameter must be > 0';
 LEAVE proc;
 END IF;
 IF min_dollar_amount_purchased = 0.00 THEN
 SELECT 'Minimum monthly dollar amount purchased parameter must be > $0.00';
 LEAVE proc;
 END IF;
 SET last_month_start = DATE_SUB(CURRENT_DATE(), INTERVAL 1 MONTH);
 SET last_month_start = STR_TO_DATE(CONCAT(YEAR(last_month_start),'-',MONTH(last_month_start),'-01'),'%Y-%m-%d');
 SET last_month_end = LAST_DAY(last_month_start);
 CREATE TEMPORARY TABLE tmpCustomer (customer_id SMALLINT UNSIGNED NOT NULL PRIMARY KEY);
 INSERT INTO tmpCustomer (customer_id)
 SELECT p.customer_id 
 FROM payment AS p
 WHERE DATE(p.payment_date) BETWEEN last_month_start AND last_month_end
 GROUP BY customer_id
 HAVING SUM(p.amount) > min_dollar_amount_purchased
 AND COUNT(customer_id) > min_monthly_purchases;
 SELECT COUNT(*) FROM tmpCustomer INTO count_rewardees;
 SELECT c.* 
 FROM tmpCustomer AS t   
 INNER JOIN customer AS c ON t.customer_id = c.customer_id;
 DROP TABLE tmpCustomer;
 END */$$
DELIMITER ;

/* Function  structure for function  `get_customer_balance` */

/*!50003 DROP FUNCTION IF EXISTS `get_customer_balance` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `get_customer_balance`(p_customer_id INT, p_effective_date DATETIME) RETURNS decimal(5,2)
    READS SQL DATA
    DETERMINISTIC
BEGIN
 DECLARE v_rentfees DECIMAL(5,2); 
 DECLARE v_overfees INTEGER;      
 DECLARE v_payments DECIMAL(5,2); 
 SELECT IFNULL(SUM(film.rental_rate),0) INTO v_rentfees
 FROM film, inventory, rental
 WHERE film.film_id = inventory.film_id
 AND inventory.inventory_id = rental.inventory_id
 AND rental.rental_date <= p_effective_date
 AND rental.customer_id = p_customer_id;
 SELECT IFNULL(SUM(IF((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) > film.rental_duration,
 ((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) - film.rental_duration),0)),0) INTO v_overfees
 FROM rental, inventory, film
 WHERE film.film_id = inventory.film_id
 AND inventory.inventory_id = rental.inventory_id
 AND rental.rental_date <= p_effective_date
 AND rental.customer_id = p_customer_id;
 SELECT IFNULL(SUM(payment.amount),0) INTO v_payments
 FROM payment
 WHERE payment.payment_date <= p_effective_date
 AND payment.customer_id = p_customer_id;
 RETURN v_rentfees + v_overfees - v_payments;
 END */$$
DELIMITER ;

/* Function  structure for function  `inventory_held_by_customer` */

/*!50003 DROP FUNCTION IF EXISTS `inventory_held_by_customer` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `inventory_held_by_customer`(p_inventory_id INT) RETURNS int(11)
    READS SQL DATA
BEGIN
 DECLARE v_customer_id INT;
 DECLARE EXIT HANDLER FOR NOT FOUND RETURN NULL;
 SELECT customer_id INTO v_customer_id
 FROM rental
 WHERE return_date IS NULL
 AND inventory_id = p_inventory_id;
 RETURN v_customer_id;
 END */$$
DELIMITER ;

/* Function  structure for function  `inventory_in_stock` */

/*!50003 DROP FUNCTION IF EXISTS `inventory_in_stock` */;
DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `inventory_in_stock`(p_inventory_id INT) RETURNS tinyint(1)
    READS SQL DATA
BEGIN
 DECLARE v_rentals INT;
 DECLARE v_out     INT;
 SELECT COUNT(*) INTO v_rentals
 FROM rental
 WHERE inventory_id = p_inventory_id;
 IF v_rentals = 0 THEN
 RETURN TRUE;
 END IF;
 SELECT COUNT(rental_id) INTO v_out
 FROM inventory LEFT JOIN rental USING(inventory_id)
 WHERE inventory.inventory_id = p_inventory_id
 AND rental.return_date IS NULL;
 IF v_out > 0 THEN
 RETURN FALSE;
 ELSE
 RETURN TRUE;
 END IF;
 END */$$
DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
