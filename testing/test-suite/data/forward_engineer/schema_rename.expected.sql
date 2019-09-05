-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema sakila_rename
-- -----------------------------------------------------
DROP SCHEMA IF EXISTS `sakila_rename` ;

-- -----------------------------------------------------
-- Schema sakila_rename
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `sakila_rename` DEFAULT CHARACTER SET latin1 ;
SHOW WARNINGS;
USE `sakila_rename` ;

-- -----------------------------------------------------
-- Table `actor`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `actor` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `actor` (
  `actor_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 201
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE INDEX `idx_actor_last_name` ON `actor` (`last_name` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `country`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `country` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `country` (
  `country_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `country` VARCHAR(50) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`country_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 110
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `city`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `city` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `city` (
  `city_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `city` VARCHAR(50) NOT NULL,
  `country_id` SMALLINT(5) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`city_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 601
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `address`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `address` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `address` (
  `address_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `address` VARCHAR(50) NOT NULL,
  `address2` VARCHAR(50) NULL DEFAULT NULL,
  `district` VARCHAR(20) NOT NULL,
  `city_id` SMALLINT(5) UNSIGNED NOT NULL,
  `postal_code` VARCHAR(10) NULL DEFAULT NULL,
  `phone` VARCHAR(20) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`address_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 606
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `category`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `category` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `category` (
  `category_id` TINYINT(3) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(25) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`category_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 17
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `staff`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `staff` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `staff` (
  `staff_id` TINYINT(3) UNSIGNED NOT NULL AUTO_INCREMENT,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `address_id` SMALLINT(5) UNSIGNED NOT NULL,
  `picture` BLOB NULL DEFAULT NULL,
  `email` VARCHAR(50) NULL DEFAULT NULL,
  `store_id` TINYINT(3) UNSIGNED NOT NULL,
  `active` TINYINT(1) NOT NULL DEFAULT '1',
  `username` VARCHAR(16) NOT NULL,
  `password` VARCHAR(40) CHARACTER SET 'utf8' COLLATE 'utf8_bin' NULL DEFAULT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`staff_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 3
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `store`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `store` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `store` (
  `store_id` TINYINT(3) UNSIGNED NOT NULL AUTO_INCREMENT,
  `manager_staff_id` TINYINT(3) UNSIGNED NOT NULL,
  `address_id` SMALLINT(5) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`store_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 3
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `customer`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `customer` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `customer` (
  `customer_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `store_id` TINYINT(3) UNSIGNED NOT NULL,
  `first_name` VARCHAR(45) NOT NULL,
  `last_name` VARCHAR(45) NOT NULL,
  `email` VARCHAR(50) NULL DEFAULT NULL,
  `address_id` SMALLINT(5) UNSIGNED NOT NULL,
  `active` TINYINT(1) NOT NULL DEFAULT '1',
  `create_date` DATETIME NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`customer_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 600
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE INDEX `idx_last_name` ON `customer` (`last_name` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `language`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `language` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `language` (
  `language_id` TINYINT(3) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` CHAR(20) NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`language_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 7
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `film`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `film` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `film` (
  `film_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `title` VARCHAR(255) NOT NULL,
  `description` TEXT NULL DEFAULT NULL,
  `release_year` YEAR(4) NULL DEFAULT NULL,
  `language_id` TINYINT(3) UNSIGNED NOT NULL,
  `original_language_id` TINYINT(3) UNSIGNED NULL DEFAULT NULL,
  `rental_duration` TINYINT(3) UNSIGNED NOT NULL DEFAULT '3',
  `rental_rate` DECIMAL(4,2) NOT NULL DEFAULT '4.99',
  `length` SMALLINT(5) UNSIGNED NULL DEFAULT NULL,
  `replacement_cost` DECIMAL(5,2) NOT NULL DEFAULT '19.99',
  `rating` ENUM('G','PG','PG-13','R','NC-17') NULL DEFAULT 'G',
  `special_features` SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') NULL DEFAULT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 1001
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE INDEX `idx_title` ON `film` (`title` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `film_actor`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `film_actor` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `film_actor` (
  `actor_id` SMALLINT(5) UNSIGNED NOT NULL,
  `film_id` SMALLINT(5) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`actor_id`, `film_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `film_category`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `film_category` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `film_category` (
  `film_id` SMALLINT(5) UNSIGNED NOT NULL,
  `category_id` TINYINT(3) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`film_id`, `category_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `film_text`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `film_text` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `film_text` (
  `film_id` SMALLINT(6) NOT NULL,
  `title` VARCHAR(255) NOT NULL,
  `description` TEXT NULL DEFAULT NULL,
  PRIMARY KEY (`film_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE FULLTEXT INDEX `idx_title_description` ON `film_text` (`title`, `description`) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `inventory`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `inventory` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `inventory` (
  `inventory_id` MEDIUMINT(8) UNSIGNED NOT NULL AUTO_INCREMENT,
  `film_id` SMALLINT(5) UNSIGNED NOT NULL,
  `store_id` TINYINT(3) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`inventory_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 4582
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `new_table`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `new_table` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `new_table` (
  `idnew_table` VARCHAR(200) NOT NULL,
  `new_tablecol` VARCHAR(45) CHARACTER SET 'ucs2' NULL DEFAULT NULL,
  PRIMARY KEY (`idnew_table`(20)))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

SHOW WARNINGS;
CREATE INDEX `idx` ON `new_table` (`new_tablecol`(10) ASC, `idnew_table`(2) ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `new_table2`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `new_table2` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `new_table2` (
  `idnew_table2` INT(11) NOT NULL,
  PRIMARY KEY (`idnew_table2`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `rental`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `rental` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `rental` (
  `rental_id` INT(11) NOT NULL AUTO_INCREMENT,
  `rental_date` DATETIME NOT NULL,
  `inventory_id` MEDIUMINT(8) UNSIGNED NOT NULL,
  `customer_id` SMALLINT(5) UNSIGNED NOT NULL,
  `return_date` DATETIME NULL DEFAULT NULL,
  `staff_id` TINYINT(3) UNSIGNED NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`rental_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 16050
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
CREATE UNIQUE INDEX `rental_date` ON `rental` (`rental_date` ASC, `inventory_id` ASC, `customer_id` ASC) VISIBLE;

SHOW WARNINGS;

-- -----------------------------------------------------
-- Table `payment`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `payment` ;

SHOW WARNINGS;
CREATE TABLE IF NOT EXISTS `payment` (
  `payment_id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
  `customer_id` SMALLINT(5) UNSIGNED NOT NULL,
  `staff_id` TINYINT(3) UNSIGNED NOT NULL,
  `rental_id` INT(11) NULL DEFAULT NULL,
  `amount` DECIMAL(5,2) NOT NULL,
  `payment_date` DATETIME NOT NULL,
  `last_update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`payment_id`))
ENGINE = InnoDB
AUTO_INCREMENT = 16050
DEFAULT CHARACTER SET = utf8;

SHOW WARNINGS;
USE `sakila_rename`;

DELIMITER $$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `customer_create_date` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`customer_create_date`
BEFORE INSERT ON `sakila`.`customer`
FOR EACH ROW
SET NEW.create_date = NOW()$$

SHOW WARNINGS$$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `del_film` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`del_film`
AFTER DELETE ON `sakila`.`film`
FOR EACH ROW
BEGIN
    DELETE FROM film_text WHERE film_id = old.film_id;
  END$$

SHOW WARNINGS$$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `ins_film` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`ins_film`
AFTER INSERT ON `sakila`.`film`
FOR EACH ROW
BEGIN
    INSERT INTO film_text (film_id, title, description)
        VALUES (new.film_id, new.title, new.description);
  END$$

SHOW WARNINGS$$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `upd_film` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`upd_film`
AFTER UPDATE ON `sakila`.`film`
FOR EACH ROW
BEGIN
    IF (old.title != new.title) or (old.description != new.description)
    THEN
        UPDATE film_text
            SET title=new.title,
                description=new.description,
                film_id=new.film_id
        WHERE film_id=old.film_id;
    END IF;
  END$$

SHOW WARNINGS$$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `rental_date` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`rental_date`
BEFORE INSERT ON `sakila`.`rental`
FOR EACH ROW
SET NEW.rental_date = NOW()$$

SHOW WARNINGS$$

USE `sakila_rename`$$
DROP TRIGGER IF EXISTS `payment_date` $$
SHOW WARNINGS$$
USE `sakila_rename`$$
CREATE
DEFINER=`root`@`localhost`
TRIGGER `sakila`.`payment_date`
BEFORE INSERT ON `sakila`.`payment`
FOR EACH ROW
SET NEW.payment_date = NOW()$$

SHOW WARNINGS$$

DELIMITER ;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
