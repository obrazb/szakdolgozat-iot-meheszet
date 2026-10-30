SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Adatbázis: `iot_okoskaptar_db`
--

-- --------------------------------------------------------

--
-- Tábla szerkezet ehhez a táblához `teszt_adatok`
--

CREATE TABLE `teszt_adatok` (
  `id` int NOT NULL,
  `kaptar_id` int NOT NULL,
  `homerseklet` float NOT NULL,
  `kijaro` int NOT NULL DEFAULT '0',
  `frekvencia` double DEFAULT '0',
  `idobelyeg` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

--
-- Indexek a kiírt táblákhoz
--

--
-- A tábla indexei `teszt_adatok`
--
ALTER TABLE `teszt_adatok`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT a táblához `teszt_adatok`
--
ALTER TABLE `teszt_adatok`
  MODIFY `id` int NOT NULL AUTO_INCREMENT;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;