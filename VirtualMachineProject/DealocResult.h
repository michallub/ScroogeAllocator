#pragma once

enum class DealocResult {
	OK,
	NOW_IS_EMPTY_CHUNK,
	WAS_FULL_CHUNK,
	WAS_FULL_IS_EMPTY,
	BAD_ADDRESS,
	DEALOC_NOT_ALLOCATED
};